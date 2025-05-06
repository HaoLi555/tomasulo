#include <stdexcept>

#include "logger.h"
#include "processor.h"

/**
 * @brief 处理前端流出的指令
 *
 * @param inst 前端将要流出的指令（在流出buffer里面）
 * @return true 后端接受该指令
 * @return false 后端拒绝该指令
 */
bool Backend::dispatchInstruction([[maybe_unused]] const Instruction &inst) {
    if (!rob.canPush()) return false;

    // TODO: Check rob and reservation station is available for push.
    // NOTE: use getFUType to get instruction's target FU
    // NOTE: FUType::NONE only goes into ROB but not Reservation Stations
    FUType futype = getFUType(inst);
    switch (futype) {
    case FUType::ALU:
        if (rsALU.hasEmptySlot()) {
            unsigned robIdx = rob.push(inst, false);
            rsALU.insertInstruction(inst, robIdx, regFile, rob);
            regFile->markBusy(inst.getRd(), robIdx);
            return true;
        }
        break;

    case FUType::BRU:
        if (rsBRU.hasEmptySlot()) {
            unsigned robIdx = rob.push(inst, false);
            rsBRU.insertInstruction(inst, robIdx, regFile, rob);
            regFile->markBusy(inst.getRd(), robIdx);
            return true;
        }
        break;

    case FUType::DIV:
        if (rsDIV.hasEmptySlot()) {
            unsigned robIdx = rob.push(inst, false);
            rsDIV.insertInstruction(inst, robIdx, regFile, rob);
            regFile->markBusy(inst.getRd(), robIdx);
            return true;
        }
        break;

    case FUType::MUL:
        if (rsMUL.hasEmptySlot()) {
            unsigned robIdx = rob.push(inst, false);
            rsMUL.insertInstruction(inst, robIdx, regFile, rob);
            regFile->markBusy(inst.getRd(), robIdx);
            return true;
        }
        break;

    case FUType::LSU:
        if (rsLSU.hasEmptySlot()) {
            unsigned robIdx = rob.push(inst, false);
            rsLSU.insertInstruction(inst, robIdx, regFile, rob);
            regFile->markBusy(inst.getRd(), robIdx);
            return true;
        }
        break;

    case FUType::NONE:
        // NOTE: 不确定这里是否是true
        rob.push(inst, true);
        return true;

    default:
        Logger::Error("never reached in dispatchInstruction!");
        break;
    }
    return false;
}

/**
 * @brief 后端完成指令提交
 *
 * @param entry 被提交的 ROBEntry
 * @param frontend 前端，用于调用前端接口
 * @return true 提交了 EXTRA::EXIT
 * @return false 其他情况
 */
bool Backend::commitInstruction([[maybe_unused]] const ROBEntry &entry,
                                [[maybe_unused]] Frontend &frontend) {
    using namespace RV32I;

    std::stringstream ss;
    ss << entry.inst;

    Logger::Info("Committing instruction %s: ", ss.str().c_str());
    Logger::Info("ROB index: %u", rob.getPopPtr());
    Logger::Info("rd: %s, value = %u",
                 xreg_name[entry.inst.getRd()].c_str(),
                 entry.state.result);

    // Exceptions are handled here
    // You don't need to change this exception handling code.

    if (entry.state.exception) {
        Logger::Error(
            "Committing an instruction with exception, fault address = %08x",
            entry.state.result);
        Logger::Error("Committing instruction %s: ", ss.str().c_str());
        Logger::Error("ROB index: %u", rob.getPopPtr());
        throw std::runtime_error("Exception encountered");
    }

    // TODO: Commit instructions here.
    // Return true when committing EXTRA::EXIT
    // NOTE: Be careful about Store Buffer!
    // NOTE: Re-executing load instructions when it is invalidated in load
    // buffer.
    // NOTE: Be careful about flush!

    // Optional TODO: Update your BTB when necessary

    if (entry.inst == RV32I::SB || entry.inst == RV32I::SH ||
        entry.inst == RV32I::SW) {
        StoreBufferSlot stSlot = storeBuffer.front();
        bool status =
            writeMemoryHierarchy(stSlot.storeAddress, stSlot.storeData, 0xF);
        if (!status) {
            return false;  // NOTE: 这里如果写未完成，不会弹出
        } else {
            storeBuffer.pop();
            rob.pop();
        }
    } else if (entry.inst == RV32I::LB || entry.inst == RV32I::LH ||
               entry.inst == RV32I::LW || entry.inst == RV32I::LBU ||
               entry.inst == RV32I::LHU) {
        LoadBufferSlot ldSlot = loadBuffer.pop(rob.getPopPtr());
        if (!ldSlot.invalidate) {
            regFile->write(
                entry.inst.getRd(), entry.state.result, rob.getPopPtr());
            rob.pop();
        } else {
            frontend.jump(entry.inst.pc);
            flush();
        }
    }
    // NOTE: 这里要考虑到jal和jalr, 且需要注意它们涉及到寄存器的操作
    else if (entry.inst == RV32I::BEQ || entry.inst == RV32I::BGE ||
             entry.inst == RV32I::BGEU || entry.inst == RV32I::BLT ||
             entry.inst == RV32I::BLTU || entry.inst == RV32I::BNE ||
             entry.inst == RV32I::JAL || entry.inst == RV32I::JALR) {
        BpuUpdateData bpuUpdateData{};
        bpuUpdateData.pc = entry.inst.pc;
        bpuUpdateData.branchTaken = entry.state.actualTaken;
        bpuUpdateData.jumpTarget = entry.state.jumpTarget;
        frontend.bpuBackendUpdate(bpuUpdateData);
        if ((entry.inst == RV32I::JAL || entry.inst == RV32I::JALR) &&
            entry.inst.getRd() != 0) {
            regFile->write(
                entry.inst.getRd(), entry.state.result, rob.getPopPtr());
        }
        if (entry.state.mispredict) {
            // NOTE: 此处的actual表示正确的情况是应不应该，而不是表示现实的情况
            unsigned int jmpAddr = entry.state.actualTaken
                                       ? entry.state.jumpTarget
                                       : entry.inst.pc + 4;
            frontend.jump(jmpAddr);
            flush();
        } else
            rob.pop();
    } else if (entry.inst == EXTRA::EXIT) {
        rob.pop();
        return true;
    } else {
        if (entry.inst.getRd() != 0) {
            regFile->write(
                entry.inst.getRd(), entry.state.result, rob.getPopPtr());
        }
        rob.pop();
    }
    return false;
}
