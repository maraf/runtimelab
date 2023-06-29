// ================================================================================================================
// |                                       Lowering for the LLVM backend                                          |
// ================================================================================================================

#include "llvm.h"

void Llvm::AddUnhandledExceptionHandler()
{
    if (!_compiler->opts.IsReversePInvoke())
    {
        return;
    }

    BasicBlock* firstTryBlock = _compiler->fgFirstBB;
    BasicBlock* lastTryBlock = _compiler->fgLastBB;

    // Make sure the first block is not in a protected region to uphold the invariant that no
    // two such regions share the first block.
    if (firstTryBlock->hasTryIndex())
    {
        _compiler->fgEnsureFirstBBisScratch();
        firstTryBlock = _compiler->fgFirstBBScratch;
    }
    _compiler->fgFirstBBScratch = nullptr;

    // Create a block for the filter and filter handler. The handler part is unreachable, but
    // we need it for the EH table to be well-formed.
    BasicBlock* filterBlock = _compiler->fgNewBBafter(BBJ_THROW, lastTryBlock, false);
    BasicBlock* handlerBlock = _compiler->fgNewBBafter(BBJ_THROW, filterBlock, false);

    // Add the new EH region at the end, since it is the least nested, and thus should be last.
    unsigned newEhIndex = _compiler->compHndBBtabCount;
    EHblkDsc* newEhDsc = _compiler->fgAddEHTableEntry(newEhIndex);

    // Initialize the new entry.
    newEhDsc->ebdHandlerType = EH_HANDLER_FILTER;
    newEhDsc->ebdTryBeg = firstTryBlock;
    newEhDsc->ebdTryLast = lastTryBlock;
    newEhDsc->ebdFilter = filterBlock;
    newEhDsc->ebdHndBeg = handlerBlock;
    newEhDsc->ebdHndLast = handlerBlock;

    newEhDsc->ebdEnclosingTryIndex = EHblkDsc::NO_ENCLOSING_INDEX;
    newEhDsc->ebdEnclosingHndIndex = EHblkDsc::NO_ENCLOSING_INDEX;

    newEhDsc->ebdTryBegOffset = firstTryBlock->bbCodeOffs;
    newEhDsc->ebdTryEndOffset = lastTryBlock->bbCodeOffsEnd;
    newEhDsc->ebdFilterBegOffset = 0; // Filter doesn't correspond to any IL.
    newEhDsc->ebdHndBegOffset = 0; // Handler doesn't correspond to any IL.
    newEhDsc->ebdHndEndOffset = 0; // Handler doesn't correspond to any IL.

    // Set some flags on the new region. This is the same as when we set up
    // EH regions in fgFindBasicBlocks(). Note that the try has no enclosing
    // handler, and the filter with filter handler have no enclosing try.
    firstTryBlock->bbFlags |= BBF_DONT_REMOVE | BBF_TRY_BEG | BBF_IMPORTED;
    firstTryBlock->setTryIndex(newEhIndex);
    firstTryBlock->clearHndIndex();

    filterBlock->bbFlags |= BBF_DONT_REMOVE | BBF_IMPORTED;
    filterBlock->bbCatchTyp = BBCT_FILTER;
    filterBlock->clearTryIndex();
    filterBlock->setHndIndex(newEhIndex);

    handlerBlock->bbFlags |= BBF_DONT_REMOVE | BBF_IMPORTED;
    handlerBlock->bbCatchTyp = BBCT_FILTER_HANDLER;
    handlerBlock->clearTryIndex();
    handlerBlock->setHndIndex(newEhIndex);

    // Walk the user code blocks and set all blocks that don't already have a try handler
    // to point to the new try handler.
    for (BasicBlock* block : _compiler->Blocks(firstTryBlock, lastTryBlock))
    {
        if (!block->hasTryIndex())
        {
            block->setTryIndex(newEhIndex);
        }
    }

    // Walk the EH table. Make every EH entry that doesn't already have an enclosing try
    // index mark this new entry as their enclosing try index.
    for (unsigned ehIndex = 0; ehIndex < newEhIndex; ehIndex++)
    {
        EHblkDsc* ehDsc = _compiler->ehGetDsc(ehIndex);
        if (ehDsc->ebdEnclosingTryIndex == EHblkDsc::NO_ENCLOSING_INDEX)
        {
            // This EH region wasn't previously nested, but now it is.
            ehDsc->ebdEnclosingTryIndex = static_cast<unsigned short>(newEhIndex);
        }
    }

    GenTree* catchArg = new (_compiler, GT_CATCH_ARG) GenTree(GT_CATCH_ARG, TYP_REF);
    catchArg->gtFlags |= GTF_ORDER_SIDEEFF;

    GenTree* handlerCall = _compiler->gtNewHelperCallNode(CORINFO_HELP_LLVM_EH_UNHANDLED_EXCEPTION, TYP_VOID, catchArg);
    Statement* handlerStmt = _compiler->gtNewStmt(handlerCall);
    _compiler->fgInsertStmtAtEnd(filterBlock, handlerStmt);

#ifdef DEBUG
    if (_compiler->verbose)
    {
        printf("ReversePInvoke method - created additional EH descriptor EH#%u for the unhandled exception filter\n",
               newEhIndex);
        _compiler->fgDispBasicBlocks();
        _compiler->fgDispHandlerTab();
    }

    _compiler->fgVerifyHandlerTab();
#endif // DEBUG
}

//------------------------------------------------------------------------
// Convert GT_STORE_LCL_VAR and GT_LCL_VAR to use the shadow stack when the local needs to be GC tracked,
// rewrite calls that returns GC types to do so via a store to a passed in address on the shadow stack.
// Likewise, store the returned value there if required.
//
void Llvm::Lower()
{
    initializeLlvmArgInfo();
    lowerBlocks();
<<<<<<< HEAD
    lowerLocalsAfterNodes();
}

//------------------------------------------------------------------------
// lowerSpillTempsLiveAcrossSafePoints: Spill GC SDSUs live across safe points.
//
// Rewrites:
//   gcTmp = IND<ref>(...)
//           CALL ; May trigger GC
//           USE(gcTmp)
// Into:
//   gcTmp = IND<ref>(...)
//           STORE_LCL_VAR<V00>(gcTmp)
//           CALL ; May trigger GC
//           USE(LCL_VAR<V00>)
//
// Done as a full IR walk pre-pass before the general lowering since we need
// to know about all GC locals to lay out the shadow stack.
//
void Llvm::lowerSpillTempsLiveAcrossSafePoints()
{
    // Cannot use raw node pointers as their values influence hash table iteration order.
    struct DeterministicNodeHashInfo : public HashTableInfo<DeterministicNodeHashInfo>
    {
        static bool Equals(GenTree* left, GenTree* right)
        {
            return left == right;
        }

        static unsigned GetHashCode(GenTree* node)
        {
            return node->TypeGet() ^ node->OperGet();
        }
    };

    // Set of SDSUs live after the current node.
    SmallHashTable<GenTree*, unsigned, 8, DeterministicNodeHashInfo> liveGcDefs(_compiler->getAllocator(CMK_Codegen));
    ArrayStack<unsigned> spillLclsRef(_compiler->getAllocator(CMK_Codegen));
    ArrayStack<unsigned> spillLclsByref(_compiler->getAllocator(CMK_Codegen));
    ArrayStack<GenTree*> containedOperands(_compiler->getAllocator(CMK_Codegen));

    auto getSpillLcl = [&](GenTree* node) {
        var_types type = node->TypeGet();
        ClassLayout* layout = nullptr;
        unsigned lclNum = BAD_VAR_NUM;
        switch (type)
        {
            case TYP_REF:
                if (!spillLclsRef.Empty())
                {
                    lclNum = spillLclsRef.Pop();
                }
                break;
            case TYP_BYREF:
                if (!spillLclsByref.Empty())
                {
                    lclNum = spillLclsByref.Pop();
                }
                break;
            case TYP_STRUCT:
                // This case should be **very** rare if at all possible. Just use a new local.
                layout = node->GetLayout(_compiler);
                break;
            default:
                unreached();
        }

        if (lclNum == BAD_VAR_NUM)
        {
            lclNum = _compiler->lvaGrabTemp(true DEBUGARG("GC SDSU live across a safepoint"));
            _compiler->lvaGetDesc(lclNum)->lvType = type;
            if (type == TYP_STRUCT)
            {
                _compiler->lvaSetStruct(lclNum, layout, false);
            }
        }

        return lclNum;
    };

    auto releaseSpillLcl = [&](unsigned lclNum) {
        LclVarDsc* varDsc = _compiler->lvaGetDesc(lclNum);
        if (varDsc->TypeGet() == TYP_REF)
        {
            spillLclsRef.Push(lclNum);
        }
        else if (varDsc->TypeGet() == TYP_BYREF)
        {
            spillLclsByref.Push(lclNum);
        }
    };

    auto isGcTemp = [compiler = _compiler](GenTree* node) {
        if (varTypeIsGC(node) || node->TypeIs(TYP_STRUCT))
        {
            if (node->TypeIs(TYP_STRUCT))
            {
                if (node->OperIs(GT_IND))
                {
                    return false;
                }
                if (!node->GetLayout(compiler)->HasGCPtr())
                {
                    return false;
                }
            }

            // Locals are handled by the general shadow stack lowering (already "spilled" so to speak).
            // Local address nodes always point to the stack (native or shadow). Constant handles will
            // only point to immortal and immovable (frozen) objects.
            return !node->OperIsLocal() && !node->OperIs(GT_LCL_ADDR) && !node->IsIconHandle();
        }

        return false;
    };

    auto spillValue = [this, &getSpillLcl](LIR::Range& blockRange, GenTree* defNode, unsigned* pSpillLclNum) {
        if (*pSpillLclNum != BAD_VAR_NUM)
        {
            // We may have already spilled this def live across multiple safe points.
            return;
        }

        unsigned spillLclNum = getSpillLcl(defNode);
        JITDUMP("Spilling as V%02u:\n", spillLclNum);
        DISPNODE(defNode);

        GenTree* store = _compiler->gtNewTempStore(spillLclNum, defNode);
        blockRange.InsertAfter(defNode, store);

        *pSpillLclNum = spillLclNum;
    };

    for (BasicBlock* block : _compiler->Blocks())
    {
        assert(liveGcDefs.Count() == 0);
        LIR::Range& blockRange = LIR::AsRange(block);

        for (GenTree* node : blockRange)
        {
            if (node->OperIs(GT_LCLHEAP))
            {
                // Calculated here as it is needed to lay out the shadow stack.
                m_lclHeapUsed = true;
            }

            if (node->isContained())
            {
                assert(!isPotentialGcSafePoint(node));
                continue;
            }

            // Handle a special case: calls with return buffer pointers need them pinned.
            if (node->IsCall() && node->AsCall()->gtArgs.HasRetBuffer())
            {
                GenTree* retBufNode = node->AsCall()->gtArgs.GetRetBufferArg()->GetNode();
                if ((retBufNode->gtLIRFlags & LIR::Flags::Mark) != 0)
                {
                    unsigned spillLclNum;
                    liveGcDefs.TryGetValue(retBufNode, &spillLclNum);
                    spillValue(blockRange, retBufNode, &spillLclNum);
                    liveGcDefs.AddOrUpdate(retBufNode, spillLclNum);
                }
            }

            GenTree* user = node;
            while (true)
            {
                for (GenTree** use : user->UseEdges())
                {
                    GenTree* operand = *use;
                    if (operand->isContained())
                    {
                        // Operands of contained nodes are used by the containing nodes. Note this algorithm will
                        // process contained operands in an out-of-order fashion; that is ok.
                        assert(operand->OperIs(GT_FIELD_LIST));
                        containedOperands.Push(operand);
                        continue;
                    }

                    if ((operand->gtLIRFlags & LIR::Flags::Mark) != 0)
                    {
                        unsigned spillLclNum = BAD_VAR_NUM;
                        bool operandWasRemoved = liveGcDefs.TryRemove(operand, &spillLclNum);
                        assert(operandWasRemoved);

                        if (spillLclNum != BAD_VAR_NUM)
                        {
                            GenTree* lclVarNode = _compiler->gtNewLclVarNode(spillLclNum);

                            *use = lclVarNode;
                            blockRange.InsertBefore(user, lclVarNode);
                            releaseSpillLcl(spillLclNum);

                            JITDUMP("Spilled [%06u] used by [%06u] replaced with V%02u:\n",
                                    Compiler::dspTreeID(operand), Compiler::dspTreeID(user), spillLclNum);
                            DISPNODE(lclVarNode);
                        }

                        operand->gtLIRFlags &= ~LIR::Flags::Mark;
                    }
                }

                if (containedOperands.Empty())
                {
                    break;
                }

                user = containedOperands.Pop();
            }

            // Find out if we need to spill anything.
            if (isPotentialGcSafePoint(node) && (liveGcDefs.Count() != 0))
            {
                JITDUMP("\nFound a safe point with GC SDSUs live across it:\n", Compiler::dspTreeID(node));
                DISPNODE(node);

                for (auto def : liveGcDefs)
                {
                    spillValue(blockRange, def.Key(), &def.Value());
                }
            }

            // Add the value defined by this node.
            if (node->IsValue() && !node->IsUnusedValue() && isGcTemp(node))
            {
                node->gtLIRFlags |= LIR::Flags::Mark;
                liveGcDefs.AddOrUpdate(node, BAD_VAR_NUM);
            }
        }
    }
}

//------------------------------------------------------------------------
// lowerLocalsBeforeNodes: strip annotations and insert initializations.
//
// We decouple promoted structs from their field locals: for independently
// promoted ones, we treat the fields as regular temporaries; parameters are
// initialized explicitly via "STORE_LCL_VAR<field>(LCL_FLD<parent>)". For
// dependently promoted cases, we will later rewrite all fields to reference
// the parent instead. We strip the annotations in "lowerLocalsAfterNodes".
// We also determine the set of locals which will need to go on the shadow
// stack, zero-initialize them if required, and assign stack offsets.
//
void Llvm::lowerLocalsBeforeNodes()
{
    populateLlvmArgNums();

    std::vector<unsigned> shadowStackLocals;

    for (unsigned lclNum = 0; lclNum < _compiler->lvaCount; lclNum++)
    {
        LclVarDsc* varDsc = _compiler->lvaGetDesc(lclNum);

        // Initialize independently promoted field locals.
        if (varDsc->lvIsParam && (_compiler->lvaGetPromotionType(varDsc) == Compiler::PROMOTION_TYPE_INDEPENDENT))
        {
            for (unsigned index = 0; index < varDsc->lvFieldCnt; index++)
            {
                unsigned   fieldLclNum = varDsc->lvFieldLclStart + index;
                LclVarDsc* fieldVarDsc = _compiler->lvaGetDesc(fieldLclNum);
                if (fieldVarDsc->lvRefCnt(RCS_NORMAL) != 0)
                {
                    GenTree* fieldValue =
                        _compiler->gtNewLclFldNode(lclNum, fieldVarDsc->TypeGet(), fieldVarDsc->lvFldOffset);
                    initializeLocalInProlog(fieldLclNum, fieldValue);

                    fieldVarDsc->lvHasExplicitInit = true;
                }
            }
        }

        // We don't know if untracked locals are live-in/out of handlers and have to assume the worst.
        if (!varDsc->lvTracked && _compiler->ehAnyFunclets())
        {
            varDsc->lvLiveInOutOfHndlr = 1;
        }

        // GC locals needs to go on the shadow stack for the scan to find them. Locals live-in/out of handlers
        // need to be preserved after the native unwind for the funclets to be callable, thus, they too need to
        // go on the shadow stack (except for parameters to funclets, naturally).
        //
        if (!isFuncletParameter(lclNum) && (varDsc->HasGCPtr() || varDsc->lvLiveInOutOfHndlr))
        {
            if (_compiler->lvaGetPromotionType(varDsc) == Compiler::PROMOTION_TYPE_INDEPENDENT)
            {
                // The individual fields will placed on the shadow stack.
                continue;
            }
            if (_compiler->lvaIsFieldOfDependentlyPromotedStruct(varDsc))
            {
                // The fields will be referenced through the parent.
                continue;
            }

            if (varDsc->lvRefCnt() == 0)
            {
                // No need to place unreferenced temps on the shadow stack.
                continue;
            }

            // We may need to insert initialization:
            //
            //  1) Zero-init if this is a non-parameter GC local, to fullfill frontend's expectations.
            //  2) Copy the initial value if this is a parameter with the home on the shadow stack.
            //
            // TODO-LLVM: in both cases we should avoid redundant initializations using liveness
            // info (for tracked locals), sharing code with "initializeLocals" in codegen. However,
            // that is currently not possible because late liveness runs after lowering.
            //
            if (!varDsc->lvHasExplicitInit)
            {
                if (varDsc->lvIsParam)
                {
                    GenTree* initVal = _compiler->gtNewLclvNode(lclNum, varDsc->TypeGet());
                    initVal->SetRegNum(REG_LLVM);

                    initializeLocalInProlog(lclNum, initVal);
                }
                else if (!_compiler->fgVarNeedsExplicitZeroInit(lclNum, /* bbInALoop */ false, /* bbIsReturn*/ false) ||
                         varDsc->HasGCPtr())
                {
                    var_types zeroType = (varDsc->TypeGet() == TYP_STRUCT) ? TYP_INT : genActualType(varDsc);
                    initializeLocalInProlog(lclNum, _compiler->gtNewZeroConNode(zeroType));
                }
            }

            shadowStackLocals.push_back(lclNum);
        }
        else
        {
            INDEBUG(varDsc->lvOnFrame = false); // For more accurate frame layout dumping.
        }
    }

    if ((shadowStackLocals.size() == 0) && m_lclHeapUsed && doUseDynamicStackForLclHeap())
    {
        // The dynamic stack is tied to the shadow one. If we have an empty shadow frame with a non-empty dynamic one,
        // an ambiguity in what state must be released on return arises - our caller might have an empty shadow frame
        // as well, but of course we don't want to release its dynamic state accidentally. To solve this, pad out the
        // shadow frame in methods that use the dynamic stack if it is empty. The need to do this should be pretty rare
        // so it is ok to waste a shadow stack slot here.
        unsigned paddingLclNum = _compiler->lvaGrabTempWithImplicitUse(true DEBUGARG("SS padding for the dynamic stack"));
        _compiler->lvaGetDesc(paddingLclNum)->lvType = TYP_REF;
        initializeLocalInProlog(paddingLclNum, _compiler->gtNewIconNode(0, TYP_REF));

        shadowStackLocals.push_back(paddingLclNum);
    }

    assignShadowStackOffsets(shadowStackLocals);
=======
>>>>>>> origin/feature/NativeAOT-LLVM
}

// LLVM Arg layout:
//    - Shadow stack (if required)
//    - This pointer (if required)
//    - Return buffer (if required)
//    - Generic context (if required)
//    - Rest of the args passed as LLVM parameters
//
void Llvm::initializeLlvmArgInfo()
{
    if (_compiler->ehAnyFunclets())
    {
        _originalShadowStackLclNum = _compiler->lvaGrabTemp(true DEBUGARG("original shadowstack"));
        LclVarDsc* originalShadowStackVarDsc = _compiler->lvaGetDesc(_originalShadowStackLclNum);
        originalShadowStackVarDsc->lvType = TYP_I_IMPL;
        originalShadowStackVarDsc->lvCorInfoType = CORINFO_TYPE_PTR;
    }

    unsigned nextLlvmArgNum = 0;
    bool isManagedAbi = !_compiler->opts.IsReversePInvoke();

    _shadowStackLclNum = _compiler->lvaGrabTempWithImplicitUse(true DEBUGARG("shadowstack"));
    LclVarDsc* shadowStackVarDsc = _compiler->lvaGetDesc(_shadowStackLclNum);
    shadowStackVarDsc->lvType = TYP_I_IMPL;
    shadowStackVarDsc->lvCorInfoType = CORINFO_TYPE_PTR;
    if (isManagedAbi)
    {
        shadowStackVarDsc->lvLlvmArgNum = nextLlvmArgNum++;
        shadowStackVarDsc->lvIsParam = true;
    }

    if (m_info->compThisArg != BAD_VAR_NUM)
    {
        LclVarDsc* thisVarDsc = _compiler->lvaGetDesc(m_info->compThisArg);
        thisVarDsc->lvCorInfoType = toCorInfoType(thisVarDsc->TypeGet());
    }

    if (m_info->compRetBuffArg != BAD_VAR_NUM)
    {
        // The return buffer is always pinned in our calling convetion, so that we can pass it as an LLVM argument.
        LclVarDsc* retBufVarDsc = _compiler->lvaGetDesc(m_info->compRetBuffArg);
        assert(retBufVarDsc->TypeGet() == TYP_BYREF);
        retBufVarDsc->lvType = TYP_I_IMPL;
        retBufVarDsc->lvCorInfoType = CORINFO_TYPE_PTR;
    }

    if (m_info->compTypeCtxtArg != BAD_VAR_NUM)
    {
        _compiler->lvaGetDesc(m_info->compTypeCtxtArg)->lvCorInfoType = CORINFO_TYPE_PTR;
    }

    for (unsigned lclNum = 0; lclNum < m_info->compArgsCount; lclNum++)
    {
        LclVarDsc* varDsc = _compiler->lvaGetDesc(lclNum);

        if (_compiler->lvaIsImplicitByRefLocal(lclNum))
        {
            // Implicit byrefs in our calling convention always point to the stack.
            assert(varDsc->TypeGet() == TYP_BYREF);
            varDsc->lvType = TYP_I_IMPL;
            varDsc->lvCorInfoType = CORINFO_TYPE_PTR;
        }

        varDsc->lvLlvmArgNum = nextLlvmArgNum++;
    }

    _llvmArgCount = nextLlvmArgNum;
}

void Llvm::lowerBlocks()
{
    for (BasicBlock* block : _compiler->Blocks())
    {
        lowerRange(block, LIR::AsRange(block));
        block->bbFlags |= BBF_MARKED;
    }

    // Lowering may insert out-of-line throw helper blocks that must themselves be lowered. We do not
    // need a more complex "to a fixed point" loop here because lowering these throw helpers will not
    // create new blocks.
    //
    for (BasicBlock* block : _compiler->Blocks())
    {
        if ((block->bbFlags & BBF_MARKED) == 0)
        {
            lowerRange(block, LIR::AsRange(block));
        }

        block->bbFlags &= ~BBF_MARKED;
    }
}

void Llvm::lowerRange(BasicBlock* block, LIR::Range& range)
{
    m_currentBlock = block;
    m_currentRange = &range;

    for (GenTree* node : range)
    {
        lowerNode(node);
    }

    INDEBUG(range.CheckLIR(_compiler, /* checkUnusedValues */ true));

    m_currentBlock = nullptr;
    m_currentRange = nullptr;
}

void Llvm::lowerNode(GenTree* node)
{
    switch (node->OperGet())
    {
        case GT_LCL_VAR:
        case GT_LCL_FLD:
        case GT_LCL_ADDR:
        case GT_STORE_LCL_VAR:
        case GT_STORE_LCL_FLD:
            lowerLocal(node->AsLclVarCommon());
            break;

        case GT_CALL:
            lowerCall(node->AsCall());
            break;

        case GT_CATCH_ARG:
            lowerCatchArg(node);
            break;

        case GT_IND:
        case GT_BLK:
        case GT_NULLCHECK:
        case GT_STOREIND:
            lowerIndir(node->AsIndir());
            break;

        case GT_STORE_BLK:
            lowerStoreBlk(node->AsBlk());
            break;

        case GT_STORE_DYN_BLK:
            lowerStoreDynBlk(node->AsStoreDynBlk());
            break;

        case GT_DIV:
        case GT_MOD:
        case GT_UDIV:
        case GT_UMOD:
            lowerDivMod(node->AsOp());
            break;

        case GT_RETURN:
            lowerReturn(node->AsUnOp());
            break;

        case GT_LCLHEAP:
            lowerLclHeap(node->AsUnOp());
            break;

        default:
            break;
    }
}

void Llvm::lowerLocal(GenTreeLclVarCommon* node)
{
    lowerFieldOfDependentlyPromotedStruct(node);

    if (node->OperIs(GT_STORE_LCL_VAR))
    {
        lowerStoreLcl(node->AsLclVarCommon());
    }

    if (node->OperIsLocalStore() && node->TypeIs(TYP_STRUCT) && genActualTypeIsInt(node->gtGetOp1()))
    {
        node->gtGetOp1()->SetContained();
    }
}

void Llvm::lowerStoreLcl(GenTreeLclVarCommon* storeLclNode)
{
    assert(storeLclNode->OperIs(GT_STORE_LCL_VAR));
    unsigned lclNum = storeLclNode->GetLclNum();
    LclVarDsc* varDsc = _compiler->lvaGetDesc(lclNum);
    GenTree* data = storeLclNode->gtGetOp1();

    unsigned convertToStoreLclFldLclNum = BAD_VAR_NUM;
    if (varDsc->CanBeReplacedWithItsField(_compiler))
    {
        convertToStoreLclFldLclNum = varDsc->lvFieldLclStart;
    }
    else if (storeLclNode->TypeIs(TYP_STRUCT))
    {
        if (data->TypeIs(TYP_STRUCT))
        {
            LIR::Use dataUse(CurrentRange(), &storeLclNode->gtOp1, storeLclNode);
            data = normalizeStructUse(dataUse, varDsc->GetLayout());
        }
        else if (data->OperIsInitVal())
        {
            // We need the local's address to create a memset.
            convertToStoreLclFldLclNum = lclNum;
        }
    }

    if (convertToStoreLclFldLclNum != BAD_VAR_NUM)
    {
        storeLclNode->SetOper(GT_STORE_LCL_FLD);
        storeLclNode->SetLclNum(convertToStoreLclFldLclNum);
        storeLclNode->AsLclFld()->SetLclOffs(0);
        storeLclNode->AsLclFld()->SetLayout(varDsc->GetLayout());

        if (storeLclNode->IsPartialLclFld(_compiler))
        {
            storeLclNode->gtFlags |= GTF_VAR_USEASG;
        }
    }
}

void Llvm::lowerFieldOfDependentlyPromotedStruct(GenTree* node)
{
    if (node->OperIsLocal() || node->OperIs(GT_LCL_ADDR))
    {
        GenTreeLclVarCommon* lclVar = node->AsLclVarCommon();
        uint16_t             offset = lclVar->GetLclOffs();
        LclVarDsc*           varDsc = _compiler->lvaGetDesc(lclVar->GetLclNum());

        if (_compiler->lvaIsFieldOfDependentlyPromotedStruct(varDsc))
        {
            switch (node->OperGet())
            {
                case GT_LCL_VAR:
                    lclVar->SetOper(GT_LCL_FLD);
                    break;

                case GT_STORE_LCL_VAR:
                    lclVar->SetOper(GT_STORE_LCL_FLD);
                    if (lclVar->IsPartialLclFld(_compiler))
                    {
                        lclVar->gtFlags |= GTF_VAR_USEASG;
                    }
                    break;
            }

            lclVar->SetLclNum(varDsc->lvParentLcl);
            lclVar->AsLclFld()->SetLclOffs(varDsc->lvFldOffset + offset);

            if ((node->gtFlags & GTF_VAR_DEF) != 0)
            {
                if (lclVar->IsPartialLclFld(_compiler))
                {
                    node->gtFlags |= GTF_VAR_USEASG;
                }
            }
        }
    }
}

void Llvm::lowerCall(GenTreeCall* callNode)
{
    // TODO-LLVM-CQ: enable fast shadow tail calls. Requires correct ABI handling.
    assert(!callNode->IsTailCall());

    if (callNode->IsHelperCall(_compiler, CORINFO_HELP_RETHROW))
    {
        lowerRethrow(callNode);
    }
    // "gtFoldExprConst" can attach a superflous argument to the overflow helper. Remove it.
    else if (callNode->IsHelperCall(_compiler, CORINFO_HELP_OVERFLOW) && !callNode->gtArgs.IsEmpty())
    {
        // TODO-LLVM: fix upstream to not attach this argument.
        CallArg* arg = callNode->gtArgs.GetArgByIndex(0);
        CurrentRange().Remove(arg->GetNode());
        callNode->gtArgs.Remove(arg);
    }

    // Doing this early simplifies code below.
    callNode->gtArgs.MoveLateToEarly();

    if (callNode->NeedsNullCheck() || callNode->IsVirtualStub())
    {
        // Virtual stub calls: our stubs don't handle null "this", as we presume doing
        // the check here has better chances for its elimination as redundant (by LLVM).
        insertNullCheckForCall(callNode);
    }

    if (callNode->IsVirtualStub())
    {
        lowerVirtualStubCall(callNode);
    }
    else if (callNode->IsDelegateInvoke())
    {
        lowerDelegateInvoke(callNode);
    }

    lowerCallReturn(callNode);
    lowerCallToShadowStack(callNode);

    if (callNode->IsUnmanaged())
    {
        lowerUnmanagedCall(callNode);
    }

    // If there is a no return, or always throw call, delete the dead code so we can add unreachable
    // statement immediately, and not after any dead RET.
    if (_compiler->fgIsThrow(callNode) || callNode->IsNoReturn())
    {
        while (CurrentRange().LastNode() != callNode)
        {
            CurrentRange().Remove(CurrentRange().LastNode(), /* markOperandsUnused */ true);
        }

        if (!CurrentBlock()->KindIs(BBJ_THROW))
        {
            _compiler->fgConvertBBToThrowBB(CurrentBlock());
        }
    }
}

void Llvm::lowerRethrow(GenTreeCall* callNode)
{
    assert(callNode->IsHelperCall(_compiler, CORINFO_HELP_RETHROW));

    // Language in ECMA 335 I.12.4.2.8.2.2 clearly states that rethrows nested inside finallys are
    // legal, however, neither C# nor the old verification system allow this. CoreCLR behavior was
    // not tested. Implementing this would imply saving the exception object to the "original" shadow
    // frame shared between funclets. For now we punt.
    if (!_compiler->ehGetDsc(CurrentBlock()->getHndIndex())->HasCatchHandler())
    {
        IMPL_LIMITATION("Nested rethrow");
    }

    // A rethrow is a special throw that preserves the stack trace. Our helper we use for rethrow has
    // the equivalent of a managed signature "void (object*)", i. e. takes the exception object address
    // explicitly. Add it here, before the general call lowering.
    assert(callNode->gtArgs.IsEmpty());

    GenTree* excObjAddr = insertShadowStackAddr(callNode, getCatchArgOffset(), _shadowStackLclNum);
    callNode->gtArgs.PushFront(_compiler, NewCallArg::Primitive(excObjAddr, CORINFO_TYPE_PTR));
}

void Llvm::lowerCatchArg(GenTree* catchArgNode)
{
    GenTree* excObjAddr = insertShadowStackAddr(catchArgNode, getCatchArgOffset(), _shadowStackLclNum);

    catchArgNode->ChangeOper(GT_IND);
    catchArgNode->gtFlags |= GTF_IND_NONFAULTING;
    catchArgNode->AsIndir()->SetAddr(excObjAddr);
}

void Llvm::lowerIndir(GenTreeIndir* indirNode)
{
    if ((indirNode->gtFlags & GTF_IND_NONFAULTING) == 0)
    {
        _compiler->fgAddCodeRef(CurrentBlock(), _compiler->bbThrowIndex(CurrentBlock()), SCK_NULL_REF_EXCPN);
    }
}

void Llvm::lowerStoreBlk(GenTreeBlk* storeBlkNode)
{
    assert(storeBlkNode->OperIs(GT_STORE_BLK));

    GenTree* src = storeBlkNode->Data();

    if (storeBlkNode->OperIsCopyBlkOp())
    {
        storeBlkNode->SetLayout(src->GetLayout(_compiler));
    }
    else
    {
        src->SetContained();
    }

    lowerIndir(storeBlkNode);
}

void Llvm::lowerStoreDynBlk(GenTreeStoreDynBlk* storeDynBlkNode)
{
    storeDynBlkNode->Data()->SetContained();
    lowerIndir(storeDynBlkNode);
}

void Llvm::lowerDivMod(GenTreeOp* divModNode)
{
    assert(divModNode->OperIs(GT_DIV, GT_MOD, GT_UDIV, GT_UMOD));

    ExceptionSetFlags exceptions = divModNode->OperExceptions(_compiler);
    if ((exceptions & ExceptionSetFlags::DivideByZeroException) != ExceptionSetFlags::None)
    {
        _compiler->fgAddCodeRef(CurrentBlock(), _compiler->bbThrowIndex(CurrentBlock()), SCK_DIV_BY_ZERO);
    }
    if ((exceptions & ExceptionSetFlags::ArithmeticException) != ExceptionSetFlags::None)
    {
        _compiler->fgAddCodeRef(CurrentBlock(), _compiler->bbThrowIndex(CurrentBlock()), SCK_OVERFLOW);
    }
}

void Llvm::lowerReturn(GenTreeUnOp* retNode)
{
    if (retNode->TypeIs(TYP_VOID))
    {
        // Nothing to do.
        return;
    }

    GenTree* retVal = retNode->gtGetOp1();
    LIR::Use retValUse(CurrentRange(), &retNode->gtOp1, retNode);
    ClassLayout* layout =
        retNode->TypeIs(TYP_STRUCT) ? _compiler->typGetObjLayout(m_info->compMethodInfo->args.retTypeClass) : nullptr;
    if (retNode->TypeIs(TYP_STRUCT) && retVal->TypeIs(TYP_STRUCT))
    {
        normalizeStructUse(retValUse, layout);
    }

    // Morph can create pretty much any type mismatch here (struct <-> primitive, primitive <-> struct, etc).
    // Fix these by spilling to a temporary (we could do better but it is not worth it, upstream will get rid
    // of the important cases). Exclude zero-init-ed structs (codegen supports them directly).
    bool isStructZero = retNode->TypeIs(TYP_STRUCT) && retVal->IsIntegralConst(0);
    if ((retNode->TypeGet() != genActualType(retVal)) && !isStructZero)
    {
        retValUse.ReplaceWithLclVar(_compiler);

        GenTreeLclVar* lclVarNode = retValUse.Def()->AsLclVar();
        lclVarNode->SetOper(GT_LCL_FLD);
        lclVarNode->ChangeType(m_info->compRetType);
        if (layout != nullptr)
        {
            lclVarNode->AsLclFld()->SetLayout(layout);
        }
    }
}

void Llvm::lowerLclHeap(GenTreeUnOp* lclHeapNode)
{
    // TODO-LLVM: lower to the dynamic stack helper here.
    m_lclHeapUsed = true;
}

void Llvm::lowerVirtualStubCall(GenTreeCall* callNode)
{
    assert(callNode->IsVirtualStub() && (callNode->gtControlExpr == nullptr) && !callNode->NeedsNullCheck());
    //
    // We transform:
    //  Call(SS, pCell, @this, args...)
    // Into:
    //  delegate* pTarget = ResolveTarget(SS, @this, pCell)
    //  pTarget(SS, @this, args...)
    //
    LIR::Use thisArgUse(CurrentRange(), &callNode->gtArgs.GetThisArg()->EarlyNodeRef(), callNode);
    unsigned thisArgLclNum = representAsLclVar(thisArgUse);
    GenTree* thisForStub = _compiler->gtNewLclvNode(thisArgLclNum, TYP_REF);
    CurrentRange().InsertBefore(callNode, thisForStub);

    CallArg* cellArg = callNode->gtArgs.FindWellKnownArg(WellKnownArg::VirtualStubCell);
    callNode->gtArgs.Remove(cellArg);

    GenTreeCall* stubCall = _compiler->gtNewHelperCallNode(
        CORINFO_HELP_LLVM_RESOLVE_INTERFACE_CALL_TARGET, TYP_I_IMPL, thisForStub, cellArg->GetNode());
    CurrentRange().InsertBefore(callNode, stubCall);

    // This call could be indirect (in case this is shared code and the cell address needed to be resolved dynamically).
    // Discard the now-not-needed address in that case.
    if (callNode->gtCallType == CT_INDIRECT)
    {
        GenTree* addr = callNode->gtCallAddr;
        if (addr->OperIs(GT_LCL_VAR))
        {
            CurrentRange().Remove(addr);
        }
        else
        {
            addr->SetUnusedValue();
        }
    }

    // Finally, retarget our call. It is no longer VSD.
    callNode->gtCallType = CT_INDIRECT;
    callNode->gtCallAddr = stubCall;
    callNode->gtStubCallStubAddr = nullptr;
    callNode->gtCallCookie = nullptr;
    callNode->gtFlags &= ~GTF_CALL_VIRT_STUB;
    callNode->gtCallMoreFlags &= ~GTF_CALL_M_VIRTSTUB_REL_INDIRECT;

    // Lower the newly introduced stub call.
    lowerCall(stubCall);
}

void Llvm::insertNullCheckForCall(GenTreeCall* callNode)
{
    assert(callNode->gtArgs.HasThisPointer());

    CallArg* thisArg = callNode->gtArgs.GetThisArg();
    if (_compiler->fgAddrCouldBeNull(thisArg->GetNode()))
    {
        LIR::Use thisArgUse(CurrentRange(), &thisArg->EarlyNodeRef(), callNode);
        unsigned thisArgLclNum = representAsLclVar(thisArgUse);

        GenTree* thisArgNode = _compiler->gtNewLclvNode(thisArgLclNum, _compiler->lvaGetDesc(thisArgLclNum)->TypeGet());
        GenTree* thisArgNullCheck = _compiler->gtNewNullCheck(thisArgNode, CurrentBlock());
        CurrentRange().InsertBefore(callNode, thisArgNode, thisArgNullCheck);

        lowerIndir(thisArgNullCheck->AsIndir());
    }

    callNode->gtFlags &= ~GTF_CALL_NULLCHECK;
}

void Llvm::lowerDelegateInvoke(GenTreeCall* callNode)
{
    // Copy of the corresponding "Lower::LowerDelegateInvoke".
    assert(callNode->IsDelegateInvoke());

    // We're going to use the 'this' expression multiple times, so make a local to copy it.
    LIR::Use thisArgUse(CurrentRange(), &callNode->gtArgs.GetThisArg()->EarlyNodeRef(), callNode);
    unsigned delegateThisLclNum = representAsLclVar(thisArgUse);

    CORINFO_EE_INFO* eeInfo = _compiler->eeGetEEInfo();

    // Replace original expression feeding into "this" with [originalThis + offsetOfDelegateInstance].
    GenTree* delegateThis = thisArgUse.Def();
    GenTree* targetThisOffet = _compiler->gtNewIconNode(eeInfo->offsetOfDelegateInstance, TYP_I_IMPL);
    GenTree* targetThisAddr = _compiler->gtNewOperNode(GT_ADD, TYP_BYREF, delegateThis, targetThisOffet);
    GenTree* targetThis = _compiler->gtNewIndir(TYP_REF, targetThisAddr);

    // Insert the new nodes just before the call. This is important to prevent the target "this" from being
    // moved by the GC while arguments after the original "this" are being evaluated.
    CurrentRange().InsertBefore(callNode, targetThisOffet, targetThisAddr, targetThis);
    thisArgUse.ReplaceWith(targetThis);

    // This indirection will null-check the original "this".
    assert(!callNode->NeedsNullCheck());
    lowerIndir(targetThis->AsIndir());

    // The new control target is [originalThis + firstTgtOffs].
    delegateThis = _compiler->gtNewLclvNode(delegateThisLclNum, TYP_REF);
    GenTree* callTargetOffset = _compiler->gtNewIconNode(eeInfo->offsetOfDelegateFirstTarget, TYP_I_IMPL);
    GenTree* callTargetAddr = _compiler->gtNewOperNode(GT_ADD, TYP_BYREF, delegateThis, callTargetOffset);
    GenTree* callTarget = _compiler->gtNewIndir(TYP_I_IMPL, callTargetAddr, GTF_IND_NONFAULTING);
    callTarget->gtFlags |= GTF_ORDER_SIDEEFF;

    CurrentRange().InsertBefore(callNode, delegateThis, callTargetOffset, callTargetAddr, callTarget);

    callNode->gtControlExpr = callTarget;
}

void Llvm::lowerUnmanagedCall(GenTreeCall* callNode)
{
    assert(callNode->IsUnmanaged());

    if (callNode->gtCallType != CT_INDIRECT)
    {
        // We cannot easily handle varargs as we do not know which args are the fixed ones.
        assert((callNode->gtCallType == CT_USER_FUNC) && !callNode->IsVarargs());

        ArrayStack<TargetAbiType> sig(_compiler->getAllocator(CMK_Codegen));
        sig.Push(getAbiTypeForType(JITtype2varType(callNode->gtCorInfoType)));
        for (CallArg& arg : callNode->gtArgs.Args())
        {
            sig.Push(getAbiTypeForType(JITtype2varType(getLlvmArgTypeForCallArg(&arg))));
        }

        // WASM requires the callee and caller signature to match. At the LLVM level, "callee type" is the function
        // type attached of the called operand and "caller" - that of its callsite. The problem, then, is that for a
        // given module, we can only have one function declaration, thus, one callee type. And we cannot know whether
        // this type will be the right one until, in general, runtime (this is the case for WASM imports provided by
        // the host environment). Thus, to achieve the experience of runtime erros on signature mismatches, we "hide"
        // the target behind an external function from another module, turning this call into an indirect one.
        //
        // TODO-LLVM: ideally, we would use a helper function here, however, adding new LLVM-specific helpers is not
        // currently possible and so we make do with special handling in codegen.
        callNode->gtEntryPoint.handle =
            GetExternalMethodAccessor(callNode->gtCallMethHnd, &sig.BottomRef(), sig.Height());
    }

    // Insert the GC transitions if required. TODO-LLVM-CQ: batch these if there are no safe points between
    // two or more consecutive PI calls.
    if (!callNode->IsSuppressGCTransition())
    {
        assert(_compiler->opts.ShouldUsePInvokeHelpers()); // No inline transition support yet.
        assert(_compiler->lvaInlinedPInvokeFrameVar != BAD_VAR_NUM);

        // Insert CORINFO_HELP_JIT_PINVOKE_BEGIN.
        GenTreeLclFld* frameAddr = _compiler->gtNewLclVarAddrNode(_compiler->lvaInlinedPInvokeFrameVar);
        GenTreeCall* helperCall = _compiler->gtNewHelperCallNode(CORINFO_HELP_JIT_PINVOKE_BEGIN, TYP_VOID, frameAddr);
        CurrentRange().InsertBefore(callNode, frameAddr, helperCall);
        lowerLocal(frameAddr);
        lowerCall(helperCall);

        // Insert CORINFO_HELP_JIT_PINVOKE_END. No need to explicitly lower the call/local address as the
        // normal lowering loop will pick them up.
        frameAddr = _compiler->gtNewLclVarAddrNode(_compiler->lvaInlinedPInvokeFrameVar);
        helperCall = _compiler->gtNewHelperCallNode(CORINFO_HELP_JIT_PINVOKE_END, TYP_VOID, frameAddr);
        CurrentRange().InsertAfter(callNode, frameAddr, helperCall);
    }
}

//------------------------------------------------------------------------
// lowerCallToShadowStack: Initialize AbiInfo for signature building.
//
void Llvm::lowerCallToShadowStack(GenTreeCall* callNode)
{
    const HelperFuncInfo* helperInfo = nullptr;
    if (callNode->IsHelperCall())
    {
        helperInfo = &getHelperFuncInfo(_compiler->eeGetHelperNum(callNode->gtCallMethHnd));
    }

    int sigArgIdx = 0;
    for (CallArg& callArg : callNode->gtArgs.Args())
    {
        GenTree* argNode = callArg.GetNode();
        CorInfoType argSigType;
        CORINFO_CLASS_HANDLE argSigClass;
        if (helperInfo == nullptr)
        {
            if (callArg.GetWellKnownArg() == WellKnownArg::ThisPointer)
            {
                argSigType = argNode->TypeIs(TYP_REF) ? CORINFO_TYPE_CLASS : CORINFO_TYPE_BYREF;
            }
            else if ((callArg.GetWellKnownArg() == WellKnownArg::InstParam) ||
                     (callArg.GetWellKnownArg() == WellKnownArg::RetBuffer))
            {
                argSigType = CORINFO_TYPE_PTR;
            }
            else if (callArg.GetSignatureCorInfoType() != CORINFO_TYPE_UNDEF)
            {
                argSigType = callArg.GetSignatureCorInfoType();
            }
            else
            {
                assert(callArg.GetSignatureType() != TYP_I_IMPL);
                argSigType = toCorInfoType(callArg.GetSignatureType());
            }

            argSigClass = callArg.GetSignatureClassHandle();
        }
        else
        {
            argSigType = helperInfo->GetSigArgType(sigArgIdx);
            argSigClass = helperInfo->GetSigArgClass(_compiler, sigArgIdx);
        }

        if (argNode->TypeIs(TYP_STRUCT))
        {
            LIR::Use argNodeUse(CurrentRange(), &callArg.EarlyNodeRef(), callNode);
            argNode = normalizeStructUse(argNodeUse, _compiler->typGetObjLayout(argSigClass));
        }

        CorInfoType argType = getLlvmArgTypeForArg(argSigType, argSigClass);
        callArg.AbiInfo.IsPointer = argType == CORINFO_TYPE_PTR;
        callArg.AbiInfo.ArgType = JITtype2varType(argType);

        sigArgIdx++;
    }
}

// Assigns "callNode->gtCorInfoType". After this method, "gtCorInfoType" switches meaning from
// "the signature return type" to "the ABI return type".
//
void Llvm::lowerCallReturn(GenTreeCall* callNode)
{
    CorInfoType sigRetType;
    if (callNode->IsHelperCall())
    {
        sigRetType = getHelperFuncInfo(callNode->GetHelperNum()).GetSigReturnType();
    }
    else if (callNode->gtCorInfoType == CORINFO_TYPE_UNDEF)
    {
        assert(callNode->TypeGet() != TYP_I_IMPL);
        sigRetType = toCorInfoType(callNode->TypeGet());
    }
    else
    {
        sigRetType = callNode->gtCorInfoType;
    }

    callNode->gtCorInfoType = getLlvmReturnType(sigRetType, callNode->gtRetClsHnd);
}

//------------------------------------------------------------------------
// normalizeStructUse: Retype "node" to have the exact type of "layout".
//
// LLVM has a strict constraint on uses and users of structs: they must
// have the exact same type, while IR only requires "layout compatibility".
// So in lowering we retype uses (and users) to match LLVM's expectations.
//
// Arguments:
//    use    - Use of the struct node to retype
//    layout - The target layout
//
// Return Value:
//    The retyped node.
//
GenTree* Llvm::normalizeStructUse(LIR::Use& use, ClassLayout* layout)
{
    GenTree* node = use.Def();
    assert(node->TypeIs(TYP_STRUCT)); // Note on SIMD: we will support it in codegen via bitcasts.

    ClassLayout* useLayout = node->GetLayout(_compiler);

    if ((useLayout != layout) && (getLlvmTypeForStruct(useLayout) != getLlvmTypeForStruct(layout)))
    {
        switch (node->OperGet())
        {
            case GT_BLK:
                node->AsBlk()->SetLayout(layout);
                break;

            case GT_LCL_FLD:
                node->AsLclFld()->SetLayout(layout);
                break;

            case GT_CALL:
                use.ReplaceWithLclVar(_compiler);
                node = use.Def();
                FALLTHROUGH;

            case GT_LCL_VAR:
                node->SetOper(GT_LCL_FLD);
                node->AsLclFld()->SetLayout(layout);
                break;

            default:
                unreached();
        }
    }

    return node;
}

unsigned Llvm::representAsLclVar(LIR::Use& use)
{
    GenTree* node = use.Def();
    if (node->OperIs(GT_LCL_VAR))
    {
        return node->AsLclVar()->GetLclNum();
    }

    return use.ReplaceWithLclVar(_compiler);
}

GenTree* Llvm::insertShadowStackAddr(GenTree* insertBefore, ssize_t offset, unsigned shadowStackLclNum)
{
    assert(isShadowStackLocal(shadowStackLclNum));

    GenTree* shadowStackLcl = _compiler->gtNewLclvNode(shadowStackLclNum, TYP_I_IMPL);
    CurrentRange().InsertBefore(insertBefore, shadowStackLcl);

    if (offset == 0)
    {
        return shadowStackLcl;
    }

    GenTree* offsetNode = _compiler->gtNewIconNode(offset, TYP_I_IMPL);
    CurrentRange().InsertBefore(insertBefore, offsetNode);
    GenTree* addNode = _compiler->gtNewOperNode(GT_ADD, TYP_I_IMPL, shadowStackLcl, offsetNode);
    CurrentRange().InsertBefore(insertBefore, addNode);

    return addNode;
}

unsigned Llvm::getCatchArgOffset() const
{
    return 0;
}
