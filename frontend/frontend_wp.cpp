#include "processor.h"
#include "with_predict.h"

FrontendWithPredict::FrontendWithPredict(const std::vector<unsigned> &inst)
    : Frontend(inst) {
    for (auto &entry : btb) {
        entry.valid = false;
    }
}

/**
 * @brief 获取指令的分支预测结果，分支预测时需要
 *
 * @param pc 指令的pc
 * @return BranchPredictBundle 分支预测的结构
 */
BranchPredictBundle FrontendWithPredict::bpuFrontendUpdate(unsigned int pc) {
    // Optional TODO: branch predictions
    BranchPredictBundle result;
    unsigned index = (pc >> 2u) & 0x3ffu;
    if (btb[index].valid && btb[index].pc == pc && btb[index].counter > 1) {
        result.predictJump = true;
        result.predictTarget = btb[index].target;
    } else {
        result.predictJump = false;
        result.predictTarget = pc + 4;
    }
    return result;
}

/**
 * @brief 用于计算NextPC，分支预测时需要
 *
 * @param pc
 * @return unsigned
 */
unsigned FrontendWithPredict::calculateNextPC(unsigned pc) const {
    // Optional TODO: branch predictions
    unsigned next_pc;
    unsigned index = (pc >> 2u) & 0x3ffu;
    if (btb[index].valid && btb[index].pc == pc && btb[index].counter > 1) {
        next_pc = btb[index].target;
    } else {
        next_pc = pc + 4;
    }
    return next_pc;
}

/**
 * @brief 用于后端提交指令时更新分支预测器状态，分支预测时需要
 *
 * @param x
 */
void FrontendWithPredict::bpuBackendUpdate(const BpuUpdateData &x) {
    // Optional TODO: branch predictions
    unsigned index = (x.pc >> 2u) & 0x3ffu;
    if (btb[index].valid && btb[index].pc == x.pc) {
        if (x.branchTaken) {
            if (btb[index].counter == 0)
                btb[index].counter = 1;
            else
                btb[index].counter = 3;
        } else {
            if (btb[index].counter == 3)
                btb[index].counter = 2;
            else
                btb[index].counter = 0;
        }
    } else {
        btb[index].valid = true;
        btb[index].pc = x.pc;
        btb[index].target = x.jumpTarget;
        btb[index].counter = 1;
    }
}

void FrontendWithPredict::reset(const std::vector<unsigned int> &inst,
                                unsigned int entry) {
    Frontend::reset(inst, entry);

    for (auto &entry : btb) {
        entry.valid = false;
    }

    // Optional TODO: Do your reset here
}
