#pragma once
#include <algorithm>
#include <memory>
#include <sstream>

#include "instructions.h"
#include "issue_slot.h"
#include "logger.h"
#include "register_file.h"
#include "rob.h"

template <unsigned size>
class ReservationStation {
    IssueSlot buffer[size];

public:
    ReservationStation();
    [[nodiscard]] bool hasEmptySlot() const;
    void insertInstruction(const Instruction &inst,
                           unsigned robIdx,
                           RegisterFile *regFile,
                           const ReorderBuffer &reorderBuffer);
    void wakeup(const ROBStatusWritePort &x);
    [[nodiscard]] bool canIssue() const;
    IssueSlot issue();
    void flush();
};

template <unsigned size>
ReservationStation<size>::ReservationStation() {
    for (auto &slot : buffer) {
        slot.busy = false;
    }
}

template <unsigned size>
bool ReservationStation<size>::hasEmptySlot() const {
    return std::any_of(buffer, buffer + size, [](const IssueSlot &slot) {
        return !slot.busy;
    });
}

template <unsigned size>
void ReservationStation<size>::insertInstruction(
    [[maybe_unused]] const Instruction &inst,
    [[maybe_unused]] unsigned robIdx,
    [[maybe_unused]] RegisterFile *const regFile,
    [[maybe_unused]] const ReorderBuffer &reorderBuffer) {
    for (auto &slot : buffer) {
        if (slot.busy) continue;
        // TODO: Dispatch instruction to this slot
        slot.inst = inst;
        slot.robIdx = robIdx;
        slot.busy = true;
        unsigned rs1 = inst.getRs1();
        unsigned rs2 = inst.getRs2();
        // NOTE: IMPORTANT
        if (regFile->isBusy(rs1)) {
            if (reorderBuffer.checkReady(regFile->getBusyIndex(rs1))) {
                slot.readPort1.value =
                    reorderBuffer.read(regFile->getBusyIndex(rs1));
                slot.readPort1.waitForWakeup = false;
                slot.readPort1.robIdx = regFile->getBusyIndex(rs1);
            } else {
                slot.readPort1.waitForWakeup = true;
                slot.readPort1.robIdx = regFile->getBusyIndex(rs1);
                slot.readPort1.value = 0;
            }
        } else {
            slot.readPort1.waitForWakeup = false;
            slot.readPort1.robIdx = 0;
            slot.readPort1.value = regFile->read(rs1);
        }

        if (regFile->isBusy(rs2)) {
            if (reorderBuffer.checkReady(regFile->getBusyIndex(rs2))) {
                slot.readPort2.value =
                    reorderBuffer.read(regFile->getBusyIndex(rs2));
                slot.readPort2.waitForWakeup = false;
                slot.readPort2.robIdx = regFile->getBusyIndex(rs2);
            } else {
                slot.readPort2.waitForWakeup = true;
                slot.readPort2.robIdx = regFile->getBusyIndex(rs2);
                slot.readPort2.value = 0;
            }
        } else {
            slot.readPort2.waitForWakeup = false;
            slot.readPort2.robIdx = 0;
            slot.readPort2.value = regFile->read(rs2);
        }
        break;
    }
}

template <unsigned size>
void ReservationStation<size>::wakeup(
    [[maybe_unused]] const ROBStatusWritePort &x) {
    // TODO: Wakeup instructions according to ROB Write
    for (auto &slot : buffer) {
        if (slot.busy) {
            if (slot.readPort1.waitForWakeup &&
                slot.readPort1.robIdx == x.robIdx) {
                slot.readPort1.waitForWakeup = false;
                slot.readPort1.value = x.result;
            }
            if (slot.readPort2.waitForWakeup &&
                slot.readPort2.robIdx == x.robIdx) {
                slot.readPort2.waitForWakeup = false;
                slot.readPort2.value = x.result;
            }
        }
    }
}

template <unsigned size>
bool ReservationStation<size>::canIssue() const {
    // TODO: Decide whether an issueSlot is ready to issue.
    // Warning: Store instructions must be issued in order!!
    if (!buffer[0].busy) {
        return false;
    }
    // store
    if (buffer[0].inst == RV32I::SB || buffer[0].inst == RV32I::SH ||
        buffer[0].inst == RV32I::SW) {
        if (buffer[0].readPort1.waitForWakeup ||
            buffer[0].readPort2.waitForWakeup) {
            return false;
        } else {
            return true;
        }
    }
    for (unsigned i = 0; i < size; i++) {
        if (buffer[i].busy && !buffer[i].readPort1.waitForWakeup &&
            !buffer[i].readPort2.waitForWakeup) {
            return true;
        }
    }
    return false;
}

template <unsigned size>
IssueSlot ReservationStation<size>::issue() {
    // TODO: Issue a ready issue slot and remove it from reservation station.
    // Warning: Store instructions must be issued in order!!
    IssueSlot result;
    int index = -1;
    if (buffer[0].inst == RV32I::SB || buffer[0].inst == RV32I::SH ||
        buffer[0].inst == RV32I::SW) {
        if (!buffer[0].busy || buffer[0].readPort1.waitForWakeup ||
            buffer[0].readPort2.waitForWakeup) {
            std::__throw_runtime_error("Store instruction not ready to issue!");
        }
        result = buffer[0];
        index = 0;
    } else {
        bool not_ready = true;
        for (unsigned i = 0; i < size; i++) {
            if (buffer[i].busy && !buffer[i].readPort1.waitForWakeup &&
                !buffer[i].readPort2.waitForWakeup) {
                result = buffer[i];
                index = i;
                not_ready = false;
                break;
            }
        }
        if (not_ready) {
            std::__throw_runtime_error("No instruction ready to issue!");
        }
    }
    unsigned i = index;
    for (; i < size - 1; i++) {
        if (buffer[i + 1].busy) {
            buffer[i] = buffer[i + 1];
        } else {
            break;
        }
    }
    buffer[i].busy = false;
    return result;
}

template <unsigned size>
void ReservationStation<size>::flush() {
    for (auto &slot : buffer) {
        slot.busy = false;
    }
}