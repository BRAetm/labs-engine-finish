/* Auto-decompiled from Helios.exe using Ghidra headless */
/* Total functions: 252 */

// Function: FUN_140001010 @ 140001010
// Signature: undefined8 * FUN_140001010(undefined8 *param_1,longlong param_2);


undefined8 * FUN_140001010(undefined8 *param_1,longlong param_2)

{
  *param_1 = std::exception::vftable;
  *(undefined1 (*) [16])(param_1 + 1) = (undefined1  [16])0x0;
  __std_exception_copy(param_2 + 8);
  return param_1;
}



// Function: FUN_140001050 @ 140001050
// Signature: char * FUN_140001050(longlong param_1);


char * FUN_140001050(longlong param_1)

{
  char *pcVar1;
  
  pcVar1 = "Unknown exception";
  if (*(char **)(param_1 + 8) != (char *)0x0) {
    pcVar1 = *(char **)(param_1 + 8);
  }
  return pcVar1;
}



// Function: FUN_140001070 @ 140001070
// Signature: undefined8 * FUN_140001070(undefined8 *param_1,ulonglong param_2);


undefined8 * FUN_140001070(undefined8 *param_1,ulonglong param_2)

{
  *param_1 = std::exception::vftable;
  __std_exception_destroy(param_1 + 1);
  if ((param_2 & 1) != 0) {
    free(param_1);
  }
  return param_1;
}



// Function: FUN_1400010e0 @ 1400010e0
// Signature: undefined8 * FUN_1400010e0(undefined8 *param_1);


undefined8 * FUN_1400010e0(undefined8 *param_1)

{
  param_1[2] = 0;
  param_1[1] = "bad array new length";
  *param_1 = std::bad_array_new_length::vftable;
  return param_1;
}



// Function: FUN_140001110 @ 140001110
// Signature: void FUN_140001110(void);


void FUN_140001110(void)

{
  undefined8 local_28 [5];
  
  FUN_1400010e0(local_28);
                    /* WARNING: Subroutine does not return */
  _CxxThrowException(local_28,(ThrowInfo *)&DAT_14000c300);
}



// Function: FUN_140001130 @ 140001130
// Signature: undefined8 * FUN_140001130(undefined8 *param_1,longlong param_2);


undefined8 * FUN_140001130(undefined8 *param_1,longlong param_2)

{
  *param_1 = std::exception::vftable;
  *(undefined1 (*) [16])(param_1 + 1) = (undefined1  [16])0x0;
  __std_exception_copy(param_2 + 8);
  *param_1 = std::bad_array_new_length::vftable;
  return param_1;
}



// Function: FUN_140001170 @ 140001170
// Signature: undefined8 * FUN_140001170(undefined8 *param_1,longlong param_2);


undefined8 * FUN_140001170(undefined8 *param_1,longlong param_2)

{
  *param_1 = std::exception::vftable;
  *(undefined1 (*) [16])(param_1 + 1) = (undefined1  [16])0x0;
  __std_exception_copy(param_2 + 8);
  *param_1 = std::bad_alloc::vftable;
  return param_1;
}



// Function: FUN_1400011b0 @ 1400011b0
// Signature: void FUN_1400011b0(void);


void FUN_1400011b0(void)

{
  code *pcVar1;
  
  std::_Xlength_error("string too long");
  pcVar1 = (code *)swi(3);
  (*pcVar1)();
  return;
}



// Function: FUN_1400011d0 @ 1400011d0
// Signature: undefined8 * FUN_1400011d0(undefined8 *param_1);


undefined8 * FUN_1400011d0(undefined8 *param_1)

{
  param_1[2] = 0;
  param_1[1] = "bad cast";
  *param_1 = std::bad_cast::vftable;
  return param_1;
}



// Function: FUN_140001200 @ 140001200
// Signature: void FUN_140001200(void);


void FUN_140001200(void)

{
  undefined8 local_28 [5];
  
  FUN_1400011d0(local_28);
                    /* WARNING: Subroutine does not return */
  _CxxThrowException(local_28,(ThrowInfo *)&DAT_14000c240);
}



// Function: FUN_140001220 @ 140001220
// Signature: undefined8 * FUN_140001220(undefined8 *param_1,longlong param_2);


undefined8 * FUN_140001220(undefined8 *param_1,longlong param_2)

{
  *param_1 = std::exception::vftable;
  *(undefined1 (*) [16])(param_1 + 1) = (undefined1  [16])0x0;
  __std_exception_copy(param_2 + 8);
  *param_1 = std::bad_cast::vftable;
  return param_1;
}



// Function: FUN_1400012a0 @ 1400012a0
// Signature: undefined1 (*) [16] FUN_1400012a0(undefined1 (*param_1) [16]);


undefined1 (*) [16] FUN_1400012a0(undefined1 (*param_1) [16])

{
  code *pcVar1;
  ulonglong uVar2;
  void *pvVar3;
  undefined1 (*pauVar4) [32];
  longlong lVar5;
  undefined1 (*pauVar6) [16];
  __uint64 _Var7;
  undefined1 *puVar8;
  ulonglong uVar9;
  void *pvVar10;
  undefined1 (*pauVar11) [32];
  undefined1 local_258 [16];
  ulonglong local_248;
  ulonglong local_240;
  WCHAR local_228 [264];
  
  pvVar10 = (void *)0x0;
  memset(local_228,0,0x208);
  GetModuleFileNameW((HMODULE)0x0,local_228,0x104);
  local_258 = (undefined1  [16])0x0;
  local_258._8_8_ = 0;
  local_248 = 0;
  local_240 = 0;
  uVar9 = 0xffffffffffffffff;
  do {
    uVar9 = uVar9 + 1;
  } while (local_228[uVar9] != L'\0');
  if (0x7ffffffffffffffe < uVar9) {
    FUN_1400011b0();
    pcVar1 = (code *)swi(3);
    pauVar6 = (undefined1 (*) [16])(*pcVar1)();
    return pauVar6;
  }
  if (uVar9 < 8) {
    local_240 = 7;
    local_248 = uVar9;
    memcpy(local_258,local_228,uVar9 * 2);
    *(undefined2 *)(local_258 + uVar9 * 2) = 0;
  }
  else {
    uVar2 = uVar9 | 7;
    if (uVar2 < 0x7fffffffffffffff) {
      if (uVar2 < 10) {
        uVar2 = 10;
      }
      if (0x7fffffffffffffff < uVar2 + 1) goto LAB_14000158b;
      _Var7 = (uVar2 + 1) * 2;
      if (_Var7 != 0) goto LAB_140001381;
    }
    else {
      _Var7 = 0xfffffffffffffffe;
      uVar2 = 0x7ffffffffffffffe;
LAB_140001381:
      if (_Var7 < 0x1000) {
        pvVar10 = operator_new(_Var7);
      }
      else {
        if (_Var7 + 0x27 <= _Var7) {
LAB_14000158b:
          FUN_140001110();
          pcVar1 = (code *)swi(3);
          pauVar6 = (undefined1 (*) [16])(*pcVar1)();
          return pauVar6;
        }
        pvVar3 = operator_new(_Var7 + 0x27);
        if (pvVar3 == (void *)0x0) goto LAB_14000156f;
        pvVar10 = (void *)((longlong)pvVar3 + 0x27U & 0xffffffffffffffe0);
        *(void **)((longlong)pvVar10 - 8) = pvVar3;
      }
    }
    local_258._0_8_ = pvVar10;
    local_248 = uVar9;
    local_240 = uVar2;
    memcpy(pvVar10,local_228,uVar9 * 2);
    *(undefined2 *)(uVar9 * 2 + (longlong)pvVar10) = 0;
  }
  pauVar11 = (undefined1 (*) [32])local_258;
  if (7 < local_240) {
    pauVar11 = (undefined1 (*) [32])local_258._0_8_;
  }
  if (local_248 == 0) {
LAB_14000150f:
    *param_1 = (undefined1  [16])0x0;
    *(undefined8 *)param_1[1] = 0;
    *(undefined8 *)(param_1[1] + 8) = 0;
    FUN_140004e50((undefined8 *)param_1,&DAT_14000aaec,0);
    if (local_240 < 8) {
      return param_1;
    }
    pvVar10 = (void *)local_258._0_8_;
    if (local_240 * 2 + 2 < 0x1000) goto LAB_1400014e6;
    pvVar10 = *(void **)(local_258._0_8_ + -8);
    lVar5 = local_258._0_8_ - (longlong)pvVar10;
  }
  else {
    lVar5 = -1;
    if (local_248 - 1 != -1) {
      lVar5 = local_248 - 1;
    }
    pauVar4 = thunk_FUN_140007040(pauVar11,(undefined1 (*) [32])(*pauVar11 + lVar5 * 2 + 2),0x5c);
    if ((pauVar4 == (undefined1 (*) [32])(*pauVar11 + lVar5 * 2 + 2)) ||
       (uVar9 = (longlong)pauVar4 - (longlong)pauVar11 >> 1, uVar9 == 0xffffffffffffffff))
    goto LAB_14000150f;
    *param_1 = (undefined1  [16])0x0;
    *(undefined8 *)param_1[1] = 0;
    *(undefined8 *)(param_1[1] + 8) = 0;
    if (local_248 < uVar9) {
      uVar9 = local_248;
    }
    puVar8 = local_258;
    if (7 < local_240) {
      puVar8 = (undefined1 *)local_258._0_8_;
    }
    FUN_140004e50((undefined8 *)param_1,puVar8,uVar9);
    if (local_240 < 8) {
      return param_1;
    }
    pvVar10 = (void *)local_258._0_8_;
    if (local_240 * 2 + 2 < 0x1000) goto LAB_1400014e6;
    pvVar10 = *(void **)(local_258._0_8_ + -8);
    lVar5 = local_258._0_8_ - (longlong)pvVar10;
  }
  if (0x1f < lVar5 - 8U) {
LAB_14000156f:
                    /* WARNING: Subroutine does not return */
    _invoke_watson((wchar_t *)0x0,(wchar_t *)0x0,(wchar_t *)0x0,0,0);
  }
LAB_1400014e6:
  free(pvVar10);
  return param_1;
}



// Function: FUN_1400015a0 @ 1400015a0
// Signature: void FUN_1400015a0(undefined8 *param_1);


/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_1400015a0(undefined8 *param_1)

{
  wchar_t *pwVar1;
  longlong lVar2;
  code *pcVar3;
  size_t sVar4;
  undefined1 auVar5 [16];
  undefined8 uVar6;
  BOOL BVar7;
  void *pvVar8;
  HANDLE hFindFile;
  undefined1 (*pauVar9) [16];
  __uint64 _Var10;
  LPCWSTR pWVar11;
  undefined1 auVar12 [8];
  ulonglong uVar13;
  undefined1 *puVar14;
  undefined1 *puVar15;
  ulonglong uVar16;
  size_t sVar17;
  undefined8 *puVar18;
  undefined1 local_2f8 [16];
  undefined1 auStack_2e8 [16];
  undefined1 local_2d8 [8];
  undefined8 uStack_2d0;
  undefined1 local_2c8 [16];
  undefined1 local_2b8 [16];
  undefined1 local_2a8 [16];
  undefined1 local_298 [16];
  undefined1 auStack_288 [16];
  _WIN32_FIND_DATAW local_278;
  
  puVar15 = (undefined1 *)0x0;
  lVar2 = param_1[2];
  if (0x7ffffffffffffffeU - lVar2 < 0x14) {
LAB_140001c0a:
    FUN_1400011b0();
LAB_140001c10:
    FUN_1400011b0();
  }
  else {
    puVar18 = param_1;
    if (7 < (ulonglong)param_1[3]) {
      puVar18 = (undefined8 *)*param_1;
    }
    local_2b8 = (undefined1  [16])0x0;
    local_2b8._8_8_ = 0;
    local_2a8 = (undefined1  [16])0x0;
    uVar16 = lVar2 + 0x14;
    uVar13 = 7;
    puVar14 = local_2b8;
    if (uVar16 < 8) {
LAB_140001697:
      local_2a8._8_8_ = uVar13;
      local_2a8._0_8_ = uVar16;
      sVar4 = lVar2 * 2;
      memcpy(puVar14,puVar18,sVar4);
      pwVar1 = (wchar_t *)(puVar14 + sVar4);
      *(undefined8 *)pwVar1 = u__pending_update_json_14000ab00._0_8_;
      *(undefined8 *)(pwVar1 + 4) = u__pending_update_json_14000ab00._8_8_;
      *(undefined8 *)(pwVar1 + 8) = u__pending_update_json_14000ab00._16_8_;
      *(undefined8 *)(pwVar1 + 0xc) = u__pending_update_json_14000ab00._24_8_;
      *(undefined8 *)(puVar14 + sVar4 + 0x20) = u__pending_update_json_14000ab00._32_8_;
      *(undefined2 *)(puVar14 + uVar16 * 2) = 0;
      pWVar11 = (LPCWSTR)local_2b8;
      if (7 < (ulonglong)local_2a8._8_8_) {
        pWVar11 = (LPCWSTR)local_2b8._0_8_;
      }
      DeleteFileW(pWVar11);
      lVar2 = param_1[2];
      if (5 < 0x7ffffffffffffffeU - lVar2) {
        puVar18 = param_1;
        if (7 < (ulonglong)param_1[3]) {
          puVar18 = (undefined8 *)*param_1;
        }
        _local_2d8 = (undefined1  [16])0x0;
        uStack_2d0 = 0;
        local_2c8 = (undefined1  [16])0x0;
        uVar16 = lVar2 + 6;
        uVar13 = 7;
        puVar14 = local_2d8;
        if (7 < uVar16) {
          uVar13 = uVar16 | 7;
          if (uVar13 < 0x7fffffffffffffff) {
            if (uVar13 < 10) {
              uVar13 = 10;
            }
            if (0x7fffffffffffffff < uVar13 + 1) goto LAB_140001c1c;
            _Var10 = (uVar13 + 1) * 2;
            if (_Var10 != 0) goto LAB_14000176a;
          }
          else {
            uVar13 = 0x7ffffffffffffffe;
            _Var10 = 0xfffffffffffffffe;
LAB_14000176a:
            if (_Var10 < 0x1000) {
              puVar15 = (undefined1 *)operator_new(_Var10);
            }
            else {
              if (_Var10 + 0x27 <= _Var10) goto LAB_140001c1c;
              pvVar8 = operator_new(_Var10 + 0x27);
              if (pvVar8 == (void *)0x0) goto LAB_140001b2c;
              puVar15 = (undefined1 *)((longlong)pvVar8 + 0x27U & 0xffffffffffffffe0);
              *(void **)(puVar15 + -8) = pvVar8;
            }
          }
          local_2d8 = (undefined1  [8])puVar15;
          puVar14 = puVar15;
        }
        local_2c8._8_8_ = uVar13;
        local_2c8._0_8_ = uVar16;
        sVar4 = lVar2 * 2;
        memcpy(puVar14,puVar18,sVar4);
        *(undefined8 *)(puVar14 + sVar4) = DAT_14000ab30;
        *(undefined4 *)(puVar14 + sVar4 + 8) = DAT_14000ab38;
        *(undefined2 *)(puVar14 + uVar16 * 2) = 0;
        auVar12 = (undefined1  [8])local_2d8;
        if (7 < (ulonglong)local_2c8._8_8_) {
          auVar12 = local_2d8;
        }
        hFindFile = FindFirstFileW((LPCWSTR)auVar12,&local_278);
        if (hFindFile != (HANDLE)0xffffffffffffffff) {
          do {
            if (((byte)local_278.dwFileAttributes & 0x10) == 0) {
              lVar2 = param_1[2];
              if (lVar2 == 0x7ffffffffffffffe) {
                FUN_1400011b0();
                goto LAB_140001c0a;
              }
              puVar18 = param_1;
              if (7 < (ulonglong)param_1[3]) {
                puVar18 = (undefined8 *)*param_1;
              }
              local_2f8 = (undefined1  [16])0x0;
              local_2f8._8_8_ = 0;
              auStack_2e8 = (undefined1  [16])0x0;
              uVar16 = lVar2 + 1;
              uVar13 = 7;
              puVar15 = local_2f8;
              if (7 < uVar16) {
                uVar13 = uVar16 | 7;
                if (uVar13 < 0x7fffffffffffffff) {
                  if (uVar13 < 10) {
                    uVar13 = 10;
                  }
                  if (0x7fffffffffffffff < uVar13 + 1) goto LAB_140001c22;
                  _Var10 = (uVar13 + 1) * 2;
                  if (_Var10 != 0) goto LAB_1400018c2;
                  puVar15 = (undefined1 *)0x0;
                }
                else {
                  uVar13 = 0x7ffffffffffffffe;
                  _Var10 = 0xfffffffffffffffe;
LAB_1400018c2:
                  if (_Var10 < 0x1000) {
                    puVar15 = (undefined1 *)operator_new(_Var10);
                  }
                  else {
                    if (_Var10 + 0x27 <= _Var10) goto LAB_140001c22;
                    pvVar8 = operator_new(_Var10 + 0x27);
                    if (pvVar8 == (void *)0x0) goto LAB_140001b45;
                    puVar15 = (undefined1 *)((longlong)pvVar8 + 0x27U & 0xffffffffffffffe0);
                    *(void **)(puVar15 + -8) = pvVar8;
                  }
                }
                local_2f8._0_8_ = puVar15;
              }
              auStack_2e8._8_8_ = uVar13;
              auStack_2e8._0_8_ = uVar16;
              sVar4 = lVar2 * 2;
              sVar17 = sVar4;
              memcpy(puVar15,puVar18,sVar4);
              *(undefined2 *)(puVar15 + sVar4) = 0x5c;
              *(undefined2 *)(puVar15 + uVar16 * 2) = 0;
              uVar16 = 0xffffffffffffffff;
              do {
                uVar13 = uVar16 + 1;
                lVar2 = uVar16 + 1;
                uVar16 = uVar13;
              } while (local_278.cFileName[lVar2] != L'\0');
              uVar6 = auStack_2e8._0_8_;
              if ((ulonglong)(auStack_2e8._8_8_ - auStack_2e8._0_8_) < uVar13) {
                pauVar9 = (undefined1 (*) [16])
                          FUN_140005d10((undefined8 *)local_2f8,uVar13,sVar17,local_278.cFileName,
                                        uVar13);
              }
              else {
                auStack_2e8._0_8_ = uVar13 + auStack_2e8._0_8_;
                puVar15 = local_2f8;
                if (7 < (ulonglong)auStack_2e8._8_8_) {
                  puVar15 = (undefined1 *)local_2f8._0_8_;
                }
                memmove(puVar15 + uVar6 * 2,local_278.cFileName,uVar13 * 2);
                *(undefined2 *)(puVar15 + (uVar6 + uVar13) * 2) = 0;
                pauVar9 = &local_2f8;
              }
              local_298 = (undefined1  [16])0x0;
              auStack_288 = (undefined1  [16])0x0;
              local_298 = *pauVar9;
              auStack_288 = pauVar9[1];
              *(undefined2 *)*pauVar9 = 0;
              *(undefined8 *)pauVar9[1] = 0;
              *(undefined8 *)(pauVar9[1] + 8) = 7;
              if (7 < (ulonglong)auStack_2e8._8_8_) {
                pvVar8 = (void *)local_2f8._0_8_;
                if ((0xfff < auStack_2e8._8_8_ * 2 + 2U) &&
                   (pvVar8 = *(void **)(local_2f8._0_8_ + -8),
                   0x1f < (ulonglong)(local_2f8._0_8_ + (-8 - (longlong)pvVar8)))) {
LAB_140001b45:
                    /* WARNING: Subroutine does not return */
                  _invoke_watson((wchar_t *)0x0,(wchar_t *)0x0,(wchar_t *)0x0,0,0);
                }
                free(pvVar8);
              }
              pWVar11 = (LPCWSTR)local_298;
              if (7 < (ulonglong)auStack_288._8_8_) {
                pWVar11 = (LPCWSTR)local_298._0_8_;
              }
              DeleteFileW(pWVar11);
              if (7 < (ulonglong)auStack_288._8_8_) {
                pvVar8 = (void *)local_298._0_8_;
                if ((0xfff < auStack_288._8_8_ * 2 + 2U) &&
                   (pvVar8 = *(void **)(local_298._0_8_ + -8),
                   0x1f < (ulonglong)(local_298._0_8_ + (-8 - (longlong)pvVar8)))) {
                    /* WARNING: Subroutine does not return */
                  _invoke_watson((wchar_t *)0x0,(wchar_t *)0x0,(wchar_t *)0x0,0,0);
                }
                free(pvVar8);
              }
            }
            BVar7 = FindNextFileW(hFindFile,&local_278);
          } while (BVar7 != 0);
          FindClose(hFindFile);
        }
        if (7 < (ulonglong)local_2c8._8_8_) {
          auVar12 = local_2d8;
          if ((0xfff < local_2c8._8_8_ * 2 + 2U) &&
             (auVar12 = *(undefined1 (*) [8])((longlong)local_2d8 + -8),
             0x1f < (ulonglong)((longlong)local_2d8 + (-8 - (longlong)auVar12)))) {
LAB_140001b2c:
                    /* WARNING: Subroutine does not return */
            _invoke_watson((wchar_t *)0x0,(wchar_t *)0x0,(wchar_t *)0x0,0,0);
          }
          free((void *)auVar12);
        }
        local_2c8._8_8_ = _UNK_14000adb8;
        local_2c8._0_8_ = _DAT_14000adb0;
        auVar5._14_2_ = 0;
        auVar5._0_14_ = stack0xfffffffffffffd2a;
        _local_2d8 = auVar5 << 0x10;
        if (7 < (ulonglong)local_2a8._8_8_) {
          pvVar8 = (void *)local_2b8._0_8_;
          if ((0xfff < local_2a8._8_8_ * 2 + 2U) &&
             (pvVar8 = *(void **)(local_2b8._0_8_ + -8),
             0x1f < (ulonglong)(local_2b8._0_8_ + (-8 - (longlong)pvVar8)))) {
LAB_140001bee:
                    /* WARNING: Subroutine does not return */
            _invoke_watson((wchar_t *)0x0,(wchar_t *)0x0,(wchar_t *)0x0,0,0);
          }
          free(pvVar8);
        }
        return;
      }
      goto LAB_140001c10;
    }
    uVar13 = uVar16 | 7;
    if (0x7ffffffffffffffe < uVar13) {
      uVar13 = 0x7ffffffffffffffe;
      _Var10 = 0xfffffffffffffffe;
LAB_140001637:
      if (_Var10 < 0x1000) {
        puVar14 = (undefined1 *)operator_new(_Var10);
      }
      else {
        if (_Var10 + 0x27 <= _Var10) goto LAB_140001c16;
        pvVar8 = operator_new(_Var10 + 0x27);
        if (pvVar8 == (void *)0x0) goto LAB_140001bee;
        puVar14 = (undefined1 *)((longlong)pvVar8 + 0x27U & 0xffffffffffffffe0);
        *(void **)(puVar14 + -8) = pvVar8;
      }
LAB_140001692:
      local_2b8._0_8_ = puVar14;
      goto LAB_140001697;
    }
    if (uVar13 < 10) {
      uVar13 = 10;
    }
    if (uVar13 + 1 < 0x8000000000000000) {
      _Var10 = (uVar13 + 1) * 2;
      puVar14 = puVar15;
      if (_Var10 != 0) goto LAB_140001637;
      goto LAB_140001692;
    }
  }
LAB_140001c16:
  FUN_140001110();
LAB_140001c1c:
  FUN_140001110();
LAB_140001c22:
  FUN_140001110();
  pcVar3 = (code *)swi(3);
  (*pcVar3)();
  return;
}



// Function: FUN_140001c30 @ 140001c30
// Signature: basic_streambuf<char,struct_std::char_traits<char>_> * FUN_140001c30(undefined8 *param_1);


/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

basic_streambuf<char,struct_std::char_traits<char>_> * FUN_140001c30(undefined8 *param_1)

{
  ulonglong uVar1;
  code *pcVar2;
  size_t _Size;
  undefined1 auVar3 [16];
  undefined1 auVar4 [16];
  undefined1 auVar5 [16];
  undefined1 auVar6 [16];
  undefined1 auVar7 [16];
  undefined1 auVar8 [16];
  byte bVar9;
  bool bVar10;
  int iVar11;
  BOOL BVar12;
  DWORD DVar13;
  void *pvVar14;
  basic_istream<char,struct_std::char_traits<char>_> *pbVar15;
  undefined1 (*pauVar16) [32];
  undefined1 (*pauVar17) [32];
  longlong lVar18;
  __uint64 _Var19;
  basic_streambuf<char,struct_std::char_traits<char>_> *pbVar20;
  undefined1 auVar21 [8];
  LPCSTR ***ppppCVar22;
  LPCWSTR pWVar23;
  LPCWSTR lpExistingFileName;
  LPCWSTR pWVar24;
  wchar_t *pwVar25;
  undefined8 uVar26;
  ulonglong uVar27;
  ulonglong uVar28;
  undefined1 (*pauVar29) [32];
  uint uVar30;
  basic_streambuf<char,struct_std::char_traits<char>_> *pbVar31;
  ulonglong _Size_00;
  undefined1 *puVar32;
  basic_streambuf<char,struct_std::char_traits<char>_> *pbVar33;
  LPCSTR pCVar34;
  undefined8 uVar35;
  ulonglong uVar36;
  longlong local_res8;
  undefined1 local_288 [16];
  ulonglong local_278;
  ulonglong local_270;
  undefined1 local_268 [16];
  undefined8 *local_258;
  undefined1 local_250 [16];
  ulonglong local_240;
  ulonglong local_238;
  undefined1 local_230 [8];
  undefined8 uStack_228;
  undefined1 auStack_220 [16];
  undefined1 local_210 [8];
  undefined8 uStack_208;
  undefined1 auStack_200 [16];
  undefined1 local_1f0 [16];
  undefined1 local_1e0 [16];
  LPCSTR **local_1d0 [2];
  int local_1c0;
  undefined4 uStack_1bc;
  ulonglong local_1b8;
  void *local_1b0;
  undefined8 uStack_1a8;
  undefined8 uStack_1a0;
  ulonglong uStack_198;
  void *local_190;
  undefined8 uStack_188;
  undefined8 uStack_180;
  ulonglong uStack_178;
  int iStack_16c;
  longlong local_168 [2];
  basic_streambuf<char,struct_std::char_traits<char>_> local_158 [24];
  longlong *local_140;
  longlong *local_120;
  int *local_108;
  undefined8 local_f0;
  undefined1 local_e8;
  undefined1 local_e7;
  undefined8 local_e4;
  undefined1 local_dc;
  FILE *local_d8;
  longlong local_d0;
  int local_c8;
  void *local_58;
  undefined8 uStack_50;
  undefined8 uStack_48;
  ulonglong uStack_40;
  
  pbVar20 = (basic_streambuf<char,struct_std::char_traits<char>_> *)0x0;
  lVar18 = param_1[2];
  if (0x7ffffffffffffffeU - lVar18 < 0x14) {
LAB_140002aa4:
    FUN_1400011b0();
    pcVar2 = (code *)swi(3);
    pbVar20 = (basic_streambuf<char,struct_std::char_traits<char>_> *)(*pcVar2)();
    return pbVar20;
  }
  if (7 < (ulonglong)param_1[3]) {
    param_1 = (undefined8 *)*param_1;
  }
  local_1f0 = (undefined1  [16])0x0;
  local_1f0._8_8_ = 0;
  local_1e0 = (undefined1  [16])0x0;
  uVar36 = lVar18 + 0x14;
  uVar27 = 7;
  pbVar31 = (basic_streambuf<char,struct_std::char_traits<char>_> *)local_1f0;
  if (7 < uVar36) {
    uVar27 = uVar36 | 7;
    if (uVar27 < 0x7fffffffffffffff) {
      if (uVar27 < 10) {
        uVar27 = 10;
      }
      if (0x7fffffffffffffff < uVar27 + 1) goto LAB_140002aaa;
      _Var19 = (uVar27 + 1) * 2;
      pbVar31 = pbVar20;
      if (_Var19 != 0) goto LAB_140001cd2;
    }
    else {
      uVar27 = 0x7ffffffffffffffe;
      _Var19 = 0xfffffffffffffffe;
LAB_140001cd2:
      if (_Var19 < 0x1000) {
        pbVar31 = (basic_streambuf<char,struct_std::char_traits<char>_> *)operator_new(_Var19);
      }
      else {
        if (_Var19 + 0x27 <= _Var19) {
LAB_140002aaa:
          FUN_140001110();
LAB_140002ab0:
          FUN_140001110();
          pcVar2 = (code *)swi(3);
          pbVar20 = (basic_streambuf<char,struct_std::char_traits<char>_> *)(*pcVar2)();
          return pbVar20;
        }
        pvVar14 = operator_new(_Var19 + 0x27);
        if (pvVar14 == (void *)0x0) goto LAB_140002a5c;
        pbVar31 = (basic_streambuf<char,struct_std::char_traits<char>_> *)
                  ((longlong)pvVar14 + 0x27U & 0xffffffffffffffe0);
        *(void **)(pbVar31 + -8) = pvVar14;
      }
    }
    local_1f0._0_8_ = pbVar31;
  }
  local_1e0._8_8_ = uVar27;
  local_1e0._0_8_ = uVar36;
  _Size = lVar18 * 2;
  memcpy(pbVar31,param_1,_Size);
  pbVar33 = pbVar31 + _Size;
  *(undefined8 *)pbVar33 = u__pending_update_json_14000ab00._0_8_;
  *(undefined8 *)(pbVar33 + 8) = u__pending_update_json_14000ab00._8_8_;
  *(undefined8 *)(pbVar33 + 0x10) = u__pending_update_json_14000ab00._16_8_;
  *(undefined8 *)(pbVar33 + 0x18) = u__pending_update_json_14000ab00._24_8_;
  *(undefined8 *)(pbVar31 + _Size + 0x20) = u__pending_update_json_14000ab00._32_8_;
  *(undefined2 *)(pbVar31 + uVar36 * 2) = 0;
  pwVar25 = (wchar_t *)local_1f0;
  if (7 < (ulonglong)local_1e0._8_8_) {
    pwVar25 = (wchar_t *)local_1f0._0_8_;
  }
  FUN_1400053a0((basic_istream<char,struct_std::char_traits<char>_> *)local_168,pwVar25);
  *(undefined ***)((longlong)local_168 + (longlong)*(int *)(local_168[0] + 4)) =
       std::basic_ifstream<char,struct_std::char_traits<char>_>::vftable;
  *(int *)((longlong)&iStack_16c + (longlong)*(int *)(local_168[0] + 4)) =
       *(int *)(local_168[0] + 4) + -0xb0;
  if (local_d8 == (FILE *)0x0) {
    pbVar31 = (basic_streambuf<char,struct_std::char_traits<char>_> *)0x1;
  }
  else {
    local_res8 = 500;
    FUN_1400038e0(&local_res8);
    local_278 = 0;
    local_270 = 0xf;
    local_288._1_15_ = SUB1615((undefined1  [16])0x0,1);
    auVar3[0xf] = 0;
    auVar3._0_15_ = local_288._1_15_;
    local_288 = auVar3 << 8;
    local_268 = (undefined1  [16])0x0;
    local_258 = (undefined8 *)0x0;
    bVar9 = std::basic_ios<char,struct_std::char_traits<char>_>::widen
                      ((basic_ios<char,struct_std::char_traits<char>_> *)
                       ((longlong)local_168 + (longlong)*(int *)(local_168[0] + 4)),'\n');
    pbVar15 = FUN_140004b60((basic_istream<char,struct_std::char_traits<char>_> *)local_168,
                            (longlong *)local_288,(ulonglong)bVar9);
    bVar10 = std::ios_base::operator_bool
                       ((ios_base *)(pbVar15 + *(int *)(*(longlong *)pbVar15 + 4)));
    uVar35 = _DAT_14000adb0;
    uVar36 = _UNK_14000adb8;
    if (bVar10) {
      do {
        pauVar29 = (undefined1 (*) [32])local_288;
        if (0xf < local_270) {
          pauVar29 = (undefined1 (*) [32])local_288._0_8_;
        }
        if (((8 < local_278) &&
            (pauVar17 = (undefined1 (*) [32])(*pauVar29 + local_278),
            pauVar16 = (undefined1 (*) [32])
                       thunk_FUN_1400072a0(pauVar29,pauVar17,(undefined1 (*) [16])"\"source\":",9),
            pauVar16 != pauVar17)) && ((longlong)pauVar16 - (longlong)pauVar29 != -1)) {
          pauVar29 = (undefined1 (*) [32])local_288;
          if (0xf < local_270) {
            pauVar29 = (undefined1 (*) [32])local_288._0_8_;
          }
          if ((local_278 == 0) ||
             (pauVar17 = (undefined1 (*) [32])(*pauVar29 + local_278),
             pauVar16 = (undefined1 (*) [32])
                        thunk_FUN_1400072a0(pauVar29,pauVar17,(undefined1 (*) [16])&DAT_14000ab4c,1)
             , pauVar16 == pauVar17)) {
            uVar27 = 0xffffffffffffffff;
          }
          else {
            uVar27 = (longlong)pauVar16 - (longlong)pauVar29;
          }
          puVar32 = local_288;
          if (0xf < local_270) {
            puVar32 = (undefined1 *)local_288._0_8_;
          }
          if (((local_278 == 0) || (local_278 - 1 < uVar27)) ||
             (pauVar29 = (undefined1 (*) [32])(puVar32 + local_278),
             pauVar17 = (undefined1 (*) [32])
                        thunk_FUN_1400072a0((undefined1 (*) [32])(puVar32 + uVar27),pauVar29,
                                            (undefined1 (*) [16])&DAT_14000ab50,1),
             pauVar17 == pauVar29)) {
            lVar18 = -1;
          }
          else {
            lVar18 = (longlong)pauVar17 - (longlong)puVar32;
          }
          uVar27 = lVar18 + 1;
          pauVar29 = (undefined1 (*) [32])local_288;
          if (0xf < local_270) {
            pauVar29 = (undefined1 (*) [32])local_288._0_8_;
          }
          if (local_278 == 0) {
            uVar28 = 0xffffffffffffffff;
          }
          else {
            lVar18 = -1;
            if (local_278 - 1 != -1) {
              lVar18 = local_278 - 1;
            }
            pauVar17 = (undefined1 (*) [32])
                       thunk_FUN_140006b40(pauVar29,(undefined1 (*) [32])(*pauVar29 + lVar18 + 1),
                                           (undefined1 (*) [16])&DAT_14000ab50,1);
            uVar28 = 0xffffffffffffffff;
            if (pauVar17 != (undefined1 (*) [32])(*pauVar29 + lVar18 + 1)) {
              uVar28 = (longlong)pauVar17 - (longlong)pauVar29;
            }
          }
          if (((uVar27 != 0xffffffffffffffff) && (uVar28 != 0xffffffffffffffff)) &&
             (uVar27 < uVar28)) {
            local_250 = (undefined1  [16])0x0;
            local_250._8_8_ = 0;
            local_240 = 0;
            local_238 = 0;
            if (local_278 < uVar27) {
              FUN_140005f10();
              goto LAB_140002aa4;
            }
            _Size_00 = uVar28 - uVar27;
            if (local_278 - uVar27 < uVar28 - uVar27) {
              _Size_00 = local_278 - uVar27;
            }
            puVar32 = local_288;
            if (0xf < local_270) {
              puVar32 = (undefined1 *)local_288._0_8_;
            }
            if (0x7fffffffffffffff < _Size_00) {
              FUN_1400011b0();
              pcVar2 = (code *)swi(3);
              pbVar20 = (basic_streambuf<char,struct_std::char_traits<char>_> *)(*pcVar2)();
              return pbVar20;
            }
            if (_Size_00 < 0x10) {
              local_238 = 0xf;
              local_240 = _Size_00;
              memcpy(local_250,puVar32 + uVar27,_Size_00);
              local_250[_Size_00] = 0;
            }
            else {
              uVar28 = _Size_00 | 0xf;
              if (uVar28 < 0x8000000000000000) {
                if (uVar28 < 0x16) {
                  uVar28 = 0x16;
                }
                uVar1 = uVar28 + 1;
                pbVar31 = pbVar20;
                if (uVar1 != 0) {
                  if (0xfff < uVar1) {
                    _Var19 = uVar28 + 0x28;
                    if (uVar1 < _Var19) goto LAB_1400020d0;
                    goto LAB_140002ab0;
                  }
                  pbVar31 = (basic_streambuf<char,struct_std::char_traits<char>_> *)
                            operator_new(uVar1);
                }
              }
              else {
                uVar28 = 0x7fffffffffffffff;
                _Var19 = 0x8000000000000027;
LAB_1400020d0:
                pvVar14 = operator_new(_Var19);
                if (pvVar14 == (void *)0x0) goto LAB_1400027fb;
                pbVar31 = (basic_streambuf<char,struct_std::char_traits<char>_> *)
                          ((longlong)pvVar14 + 0x27U & 0xffffffffffffffe0);
                *(void **)(pbVar31 + -8) = pvVar14;
              }
              local_250._0_8_ = pbVar31;
              local_240 = _Size_00;
              local_238 = uVar28;
              memcpy(pbVar31,puVar32 + uVar27,_Size_00);
              pbVar31[_Size_00] = (basic_streambuf<char,struct_std::char_traits<char>_>)0x0;
            }
            bVar9 = std::basic_ios<char,struct_std::char_traits<char>_>::widen
                              ((basic_ios<char,struct_std::char_traits<char>_> *)
                               ((longlong)local_168 + (longlong)*(int *)(local_168[0] + 4)),'\n');
            pbVar15 = FUN_140004b60((basic_istream<char,struct_std::char_traits<char>_> *)local_168,
                                    (longlong *)local_288,(ulonglong)bVar9);
            bVar10 = std::ios_base::operator_bool
                               ((ios_base *)(pbVar15 + *(int *)(*(longlong *)pbVar15 + 4)));
            if (bVar10) {
              pauVar29 = (undefined1 (*) [32])local_288;
              if (0xf < local_270) {
                pauVar29 = (undefined1 (*) [32])local_288._0_8_;
              }
              if (((8 < local_278) &&
                  (pauVar17 = (undefined1 (*) [32])(*pauVar29 + local_278),
                  pauVar16 = (undefined1 (*) [32])
                             thunk_FUN_1400072a0(pauVar29,pauVar17,
                                                 (undefined1 (*) [16])"\"target\":",9),
                  pauVar16 != pauVar17)) && ((longlong)pauVar16 - (longlong)pauVar29 != -1)) {
                pauVar29 = (undefined1 (*) [32])local_288;
                if (0xf < local_270) {
                  pauVar29 = (undefined1 (*) [32])local_288._0_8_;
                }
                if ((local_278 == 0) ||
                   (pauVar17 = (undefined1 (*) [32])(*pauVar29 + local_278),
                   pauVar16 = (undefined1 (*) [32])
                              thunk_FUN_1400072a0(pauVar29,pauVar17,
                                                  (undefined1 (*) [16])&DAT_14000ab4c,1),
                   pauVar16 == pauVar17)) {
                  uVar27 = 0xffffffffffffffff;
                }
                else {
                  uVar27 = (longlong)pauVar16 - (longlong)pauVar29;
                }
                puVar32 = local_288;
                if (0xf < local_270) {
                  puVar32 = (undefined1 *)local_288._0_8_;
                }
                if (((local_278 == 0) || (local_278 - 1 < uVar27)) ||
                   (pauVar29 = (undefined1 (*) [32])(puVar32 + local_278),
                   pauVar17 = (undefined1 (*) [32])
                              thunk_FUN_1400072a0((undefined1 (*) [32])(puVar32 + uVar27),pauVar29,
                                                  (undefined1 (*) [16])&DAT_14000ab50,1),
                   pauVar17 == pauVar29)) {
                  lVar18 = -1;
                }
                else {
                  lVar18 = (longlong)pauVar17 - (longlong)puVar32;
                }
                uVar27 = lVar18 + 1;
                pauVar29 = (undefined1 (*) [32])local_288;
                if (0xf < local_270) {
                  pauVar29 = (undefined1 (*) [32])local_288._0_8_;
                }
                if (local_278 == 0) {
                  uVar28 = 0xffffffffffffffff;
                }
                else {
                  lVar18 = -1;
                  if (local_278 - 1 != -1) {
                    lVar18 = local_278 - 1;
                  }
                  pauVar17 = (undefined1 (*) [32])
                             thunk_FUN_140006b40(pauVar29,(undefined1 (*) [32])
                                                          (*pauVar29 + lVar18 + 1),
                                                 (undefined1 (*) [16])&DAT_14000ab50,1);
                  uVar28 = 0xffffffffffffffff;
                  if (pauVar17 != (undefined1 (*) [32])(*pauVar29 + lVar18 + 1)) {
                    uVar28 = (longlong)pauVar17 - (longlong)pauVar29;
                  }
                }
                if (((uVar27 != 0xffffffffffffffff) && (uVar28 != 0xffffffffffffffff)) &&
                   (uVar27 < uVar28)) {
                  FUN_140005240((undefined1 (*) [16])local_1d0,(undefined8 *)local_288,uVar27,
                                uVar28 - uVar27);
                  if (local_240 == 0) {
                    auStack_220._8_8_ = uVar36;
                    auStack_220._0_8_ = uVar35;
                    stack0xfffffffffffffdd2 = SUB1614((undefined1  [16])0x0,2);
                    auVar4._14_2_ = 0;
                    auVar4._0_14_ = stack0xfffffffffffffdd2;
                    _local_230 = auVar4 << 0x10;
                    auVar3 = _local_230;
                    uStack_228 = 0;
                    local_58 = (void *)0x0;
                    uStack_50 = uStack_228;
                    uStack_48 = uVar35;
                    uStack_40 = uVar36;
                    _local_230 = auVar3;
                  }
                  else {
                    pCVar34 = local_250;
                    if (0xf < local_238) {
                      pCVar34 = (LPCSTR)local_250._0_8_;
                    }
                    uVar26 = 0;
                    iVar11 = MultiByteToWideChar(0xfde9,0,pCVar34,(int)local_240,(LPWSTR)0x0,0);
                    _local_230 = (undefined1  [16])0x0;
                    auStack_220 = (undefined1  [16])0x0;
                    FUN_140004d10((undefined8 *)local_230,uVar26,(longlong)iVar11);
                    auVar21 = (undefined1  [8])local_230;
                    if (7 < (ulonglong)auStack_220._8_8_) {
                      auVar21 = local_230;
                    }
                    pCVar34 = local_250;
                    if (0xf < local_238) {
                      pCVar34 = (LPCSTR)local_250._0_8_;
                    }
                    MultiByteToWideChar(0xfde9,0,pCVar34,(int)local_240,(LPWSTR)auVar21,iVar11);
                    local_58 = (void *)local_230;
                    uStack_50 = uStack_228;
                    uStack_48 = auStack_220._0_8_;
                    uStack_40 = auStack_220._8_8_;
                  }
                  if (CONCAT44(uStack_1bc,local_1c0) == 0) {
                    stack0xfffffffffffffdf2 = SUB1614((undefined1  [16])0x0,2);
                    auVar5._14_2_ = 0;
                    auVar5._0_14_ = stack0xfffffffffffffdf2;
                    _local_210 = auVar5 << 0x10;
                    auVar3 = _local_210;
                    uStack_208 = 0;
                    local_190 = (void *)0x0;
                    uStack_188 = uStack_208;
                    uStack_180 = uVar35;
                    uStack_178 = uVar36;
                    _local_210 = auVar3;
                  }
                  else {
                    ppppCVar22 = local_1d0;
                    if (0xf < local_1b8) {
                      ppppCVar22 = (LPCSTR ***)local_1d0[0];
                    }
                    uVar26 = 0;
                    iVar11 = MultiByteToWideChar(0xfde9,0,(LPCSTR)ppppCVar22,local_1c0,(LPWSTR)0x0,0
                                                );
                    _local_210 = (undefined1  [16])0x0;
                    auStack_200 = (undefined1  [16])0x0;
                    FUN_140004d10((undefined8 *)local_210,uVar26,(longlong)iVar11);
                    auVar21 = (undefined1  [8])local_210;
                    if (7 < (ulonglong)auStack_200._8_8_) {
                      auVar21 = local_210;
                    }
                    ppppCVar22 = local_1d0;
                    if (0xf < local_1b8) {
                      ppppCVar22 = (LPCSTR ***)local_1d0[0];
                    }
                    MultiByteToWideChar(0xfde9,0,(LPCSTR)ppppCVar22,local_1c0,(LPWSTR)auVar21,iVar11
                                       );
                    local_190 = (void *)local_210;
                    uStack_188 = uStack_208;
                    uStack_180 = auStack_200._0_8_;
                    uStack_178 = auStack_200._8_8_;
                  }
                  local_1b0 = local_58;
                  uStack_1a8 = uStack_50;
                  uStack_1a0 = uStack_48;
                  uStack_198 = uStack_40;
                  auVar6._14_2_ = 0;
                  auVar6._0_14_ = stack0xfffffffffffffdd2;
                  _local_230 = auVar6 << 0x10;
                  auStack_220 = ZEXT816(7) << 0x40;
                  auVar7._14_2_ = 0;
                  auVar7._0_14_ = stack0xfffffffffffffdf2;
                  _local_210 = auVar7 << 0x10;
                  auStack_200 = ZEXT816(7) << 0x40;
                  if ((undefined8 *)local_268._8_8_ == local_258) {
                    FUN_140005690((longlong *)local_268,(undefined8 *)local_268._8_8_,&local_1b0);
                    uVar27 = uStack_178;
                    uVar28 = uStack_198;
                  }
                  else {
                    *(void **)local_268._8_8_ = local_58;
                    *(undefined8 *)(local_268._8_8_ + 8) = uStack_50;
                    *(undefined8 *)(local_268._8_8_ + 0x10) = uStack_48;
                    *(ulonglong *)(local_268._8_8_ + 0x18) = uStack_40;
                    local_1b0 = (void *)((ulonglong)local_58 & 0xffffffffffff0000);
                    *(void **)(local_268._8_8_ + 0x20) = local_190;
                    *(undefined8 *)(local_268._8_8_ + 0x28) = uStack_188;
                    *(undefined8 *)(local_268._8_8_ + 0x30) = uStack_180;
                    *(ulonglong *)(local_268._8_8_ + 0x38) = uStack_178;
                    local_190 = (void *)((ulonglong)local_190 & 0xffffffffffff0000);
                    local_268._8_8_ = (undefined8 *)(local_268._8_8_ + 0x40);
                    uVar27 = 7;
                    uVar28 = 7;
                  }
                  if (7 < uVar27) {
                    pvVar14 = local_190;
                    if ((0xfff < uVar27 * 2 + 2) &&
                       (pvVar14 = *(void **)((longlong)local_190 + -8),
                       0x1f < (ulonglong)((longlong)local_190 + (-8 - (longlong)pvVar14))))
                    goto LAB_1400027c9;
                    free(pvVar14);
                  }
                  if (7 < uVar28) {
                    pvVar14 = local_1b0;
                    if ((0xfff < uVar28 * 2 + 2) &&
                       (pvVar14 = *(void **)((longlong)local_1b0 + -8),
                       0x1f < (ulonglong)((longlong)local_1b0 + (-8 - (longlong)pvVar14)))) {
LAB_1400027c9:
                    /* WARNING: Subroutine does not return */
                      _invoke_watson((wchar_t *)0x0,(wchar_t *)0x0,(wchar_t *)0x0,0,0);
                    }
                    free(pvVar14);
                  }
                  if (0xf < local_1b8) {
                    ppppCVar22 = (LPCSTR ***)local_1d0[0];
                    if ((0xfff < local_1b8 + 1) &&
                       (ppppCVar22 = (LPCSTR ***)local_1d0[0][-1],
                       0x1f < (ulonglong)((longlong)local_1d0[0] + (-8 - (longlong)ppppCVar22)))) {
                    /* WARNING: Subroutine does not return */
                      _invoke_watson((wchar_t *)0x0,(wchar_t *)0x0,(wchar_t *)0x0,0,0);
                    }
                    free(ppppCVar22);
                  }
                }
              }
            }
            if (0xf < local_238) {
              pvVar14 = (void *)local_250._0_8_;
              if ((0xfff < local_238 + 1) &&
                 (pvVar14 = *(void **)(local_250._0_8_ + -8),
                 0x1f < (ulonglong)(local_250._0_8_ + (-8 - (longlong)pvVar14)))) {
LAB_1400027fb:
                    /* WARNING: Subroutine does not return */
                _invoke_watson((wchar_t *)0x0,(wchar_t *)0x0,(wchar_t *)0x0,0,0);
              }
              free(pvVar14);
            }
          }
        }
        bVar9 = std::basic_ios<char,struct_std::char_traits<char>_>::widen
                          ((basic_ios<char,struct_std::char_traits<char>_> *)
                           ((longlong)local_168 + (longlong)*(int *)(local_168[0] + 4)),'\n');
        pbVar15 = FUN_140004b60((basic_istream<char,struct_std::char_traits<char>_> *)local_168,
                                (longlong *)local_288,(ulonglong)bVar9);
        bVar10 = std::ios_base::operator_bool
                           ((ios_base *)(pbVar15 + *(int *)(*(longlong *)pbVar15 + 4)));
      } while (bVar10);
    }
    if (local_d8 == (FILE *)0x0) {
      local_dc = 0;
      local_e7 = 0;
      std::basic_streambuf<char,struct_std::char_traits<char>_>::_Init(local_158);
LAB_140002851:
      local_d8 = (FILE *)0x0;
      local_f0 = 0;
      local_e4 = DAT_14000e938;
      std::basic_ios<char,struct_std::char_traits<char>_>::setstate
                ((basic_ios<char,struct_std::char_traits<char>_> *)
                 ((longlong)local_168 + (longlong)*(int *)(local_168[0] + 4)),2,false);
    }
    else {
      if ((undefined1 *)*local_140 == &local_e8) {
        *local_140 = local_d0;
        *local_120 = local_d0;
        *local_108 = local_c8 - (int)local_d0;
      }
      bVar10 = FUN_140005580((longlong *)local_158);
      pbVar31 = local_158;
      if (!bVar10) {
        pbVar31 = pbVar20;
      }
      iVar11 = fclose(local_d8);
      pbVar33 = pbVar20;
      if (iVar11 == 0) {
        pbVar33 = pbVar31;
      }
      local_dc = 0;
      local_e7 = 0;
      std::basic_streambuf<char,struct_std::char_traits<char>_>::_Init(local_158);
      local_d8 = (FILE *)0x0;
      local_e4 = DAT_14000e938;
      local_f0 = 0;
      if (pbVar33 == (basic_streambuf<char,struct_std::char_traits<char>_> *)0x0)
      goto LAB_140002851;
    }
    pbVar31 = (basic_streambuf<char,struct_std::char_traits<char>_> *)0x1;
    uVar35 = local_268._8_8_;
    if (local_268._0_8_ == local_268._8_8_) {
LAB_14000291c:
      pWVar24 = (LPCWSTR)local_1f0;
      if (7 < (ulonglong)local_1e0._8_8_) {
        pWVar24 = (LPCWSTR)local_1f0._0_8_;
      }
      DeleteFileW(pWVar24);
    }
    else {
      pbVar33 = pbVar20;
      pWVar24 = (LPCWSTR)(local_268._0_8_ + 0x20);
LAB_140002893:
      do {
        pWVar23 = pWVar24;
        if (7 < *(ulonglong *)(pWVar24 + 0xc)) {
          pWVar23 = *(LPCWSTR *)pWVar24;
        }
        BVar12 = DeleteFileW(pWVar23);
        if ((BVar12 == 0) && (DVar13 = GetLastError(), DVar13 != 2)) {
          local_res8 = 200;
          FUN_1400038e0(&local_res8);
          uVar30 = (int)pbVar33 + 1;
          pbVar33 = (basic_streambuf<char,struct_std::char_traits<char>_> *)(ulonglong)uVar30;
          if ((int)uVar30 < 10) goto LAB_140002893;
        }
        pWVar23 = pWVar24;
        if (7 < *(ulonglong *)(pWVar24 + 0xc)) {
          pWVar23 = *(LPCWSTR *)pWVar24;
        }
        lpExistingFileName = pWVar24 + -0x10;
        if (7 < *(ulonglong *)(pWVar24 + -4)) {
          lpExistingFileName = *(LPCWSTR *)lpExistingFileName;
        }
        BVar12 = MoveFileExW(lpExistingFileName,pWVar23,1);
        if (BVar12 == 0) {
          pbVar31 = pbVar20;
        }
        pWVar23 = pWVar24 + 0x10;
        pbVar33 = pbVar20;
        pWVar24 = pWVar24 + 0x20;
      } while (pWVar23 != (LPCWSTR)uVar35);
      if ((char)pbVar31 != '\0') goto LAB_14000291c;
    }
    if ((ulonglong *)local_268._0_8_ != (ulonglong *)0x0) {
      FUN_140005a30((ulonglong *)local_268._0_8_,(ulonglong *)local_268._8_8_);
      pvVar14 = (void *)local_268._0_8_;
      if ((0xfff < ((longlong)local_258 - local_268._0_8_ & 0xffffffffffffffc0U)) &&
         (pvVar14 = *(void **)(local_268._0_8_ + -8),
         0x1f < (ulonglong)(local_268._0_8_ + (-8 - (longlong)pvVar14)))) {
                    /* WARNING: Subroutine does not return */
        _invoke_watson((wchar_t *)0x0,(wchar_t *)0x0,(wchar_t *)0x0,0,0);
      }
      free(pvVar14);
      local_268 = (undefined1  [16])0x0;
      local_258 = (undefined8 *)0x0;
    }
    if (0xf < local_270) {
      pvVar14 = (void *)local_288._0_8_;
      if ((0xfff < local_270 + 1) &&
         (pvVar14 = *(void **)(local_288._0_8_ + -8),
         0x1f < (ulonglong)(local_288._0_8_ + (-8 - (longlong)pvVar14)))) {
                    /* WARNING: Subroutine does not return */
        _invoke_watson((wchar_t *)0x0,(wchar_t *)0x0,(wchar_t *)0x0,0,0);
      }
      free(pvVar14);
    }
    local_278 = 0;
    local_270 = 0xf;
    auVar8[0xf] = 0;
    auVar8._0_15_ = local_288._1_15_;
    local_288 = auVar8 << 8;
  }
  FUN_140002ac0(local_168);
  if (7 < (ulonglong)local_1e0._8_8_) {
    pvVar14 = (void *)local_1f0._0_8_;
    if ((0xfff < local_1e0._8_8_ * 2 + 2U) &&
       (pvVar14 = *(void **)(local_1f0._0_8_ + -8),
       0x1f < (ulonglong)(local_1f0._0_8_ + (-8 - (longlong)pvVar14)))) {
LAB_140002a5c:
                    /* WARNING: Subroutine does not return */
      _invoke_watson((wchar_t *)0x0,(wchar_t *)0x0,(wchar_t *)0x0,0,0);
    }
    free(pvVar14);
  }
  return pbVar31;
}



// Function: FUN_140002ac0 @ 140002ac0
// Signature: void FUN_140002ac0(longlong *param_1);


void FUN_140002ac0(longlong *param_1)

{
  basic_streambuf<char,struct_std::char_traits<char>_> *this;
  basic_ios<char,struct_std::char_traits<char>_> *this_00;
  longlong lVar1;
  longlong lVar2;
  
  this_00 = (basic_ios<char,struct_std::char_traits<char>_> *)(param_1 + 0x16);
  *(undefined ***)(this_00 + (longlong)*(int *)(*param_1 + 4) + -0xb0) =
       std::basic_ifstream<char,struct_std::char_traits<char>_>::vftable;
  *(int *)(this_00 + (longlong)*(int *)(*param_1 + 4) + -0xb4) = *(int *)(*param_1 + 4) + -0xb0;
  this = (basic_streambuf<char,struct_std::char_traits<char>_> *)(param_1 + 2);
  *(undefined ***)this = std::basic_filebuf<char,struct_std::char_traits<char>_>::vftable;
  if ((param_1[0x12] != 0) && (*(longlong **)param_1[5] == param_1 + 0x10)) {
    lVar1 = param_1[0x14];
    lVar2 = param_1[0x13];
    *(longlong *)param_1[5] = lVar2;
    *(longlong *)param_1[9] = lVar2;
    *(int *)param_1[0xc] = (int)lVar1 - (int)lVar2;
  }
  if (*(char *)((longlong)param_1 + 0x8c) != '\0') {
    if (param_1[0x12] != 0) {
      if (*(longlong **)param_1[5] == param_1 + 0x10) {
        lVar1 = param_1[0x14];
        lVar2 = param_1[0x13];
        *(longlong *)param_1[5] = lVar2;
        *(longlong *)param_1[9] = lVar2;
        *(int *)param_1[0xc] = (int)lVar1 - (int)lVar2;
      }
      FUN_140005580((longlong *)this);
      fclose((FILE *)param_1[0x12]);
    }
    *(undefined1 *)((longlong)param_1 + 0x8c) = 0;
    *(undefined1 *)((longlong)param_1 + 0x81) = 0;
    std::basic_streambuf<char,struct_std::char_traits<char>_>::_Init(this);
    param_1[0x12] = 0;
    *(undefined8 *)((longlong)param_1 + 0x84) = DAT_14000e938;
    param_1[0xf] = 0;
  }
  std::basic_streambuf<char,struct_std::char_traits<char>_>::
  ~basic_streambuf<char,struct_std::char_traits<char>_>(this);
  std::basic_istream<char,struct_std::char_traits<char>_>::
  ~basic_istream<char,struct_std::char_traits<char>_>
            ((basic_istream<char,struct_std::char_traits<char>_> *)(param_1 + 3));
                    /* WARNING: Could not recover jumptable at 0x000140002be1. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  std::basic_ios<char,struct_std::char_traits<char>_>::
  ~basic_ios<char,struct_std::char_traits<char>_>(this_00);
  return;
}



// Function: FUN_140002bf0 @ 140002bf0
// Signature: void FUN_140002bf0(longlong *param_1);


void FUN_140002bf0(longlong *param_1)

{
  void *pvVar1;
  void *pvVar2;
  
  if (7 < (ulonglong)param_1[7]) {
    pvVar1 = (void *)param_1[4];
    pvVar2 = pvVar1;
    if ((0xfff < param_1[7] * 2 + 2U) &&
       (pvVar2 = *(void **)((longlong)pvVar1 + -8),
       0x1f < (ulonglong)((longlong)pvVar1 + (-8 - (longlong)pvVar2)))) goto LAB_140002ca0;
    free(pvVar2);
  }
  param_1[6] = 0;
  param_1[7] = 7;
  *(undefined2 *)(param_1 + 4) = 0;
  if (7 < (ulonglong)param_1[3]) {
    pvVar1 = (void *)*param_1;
    pvVar2 = pvVar1;
    if ((0xfff < param_1[3] * 2 + 2U) &&
       (pvVar2 = *(void **)((longlong)pvVar1 + -8),
       0x1f < (ulonglong)((longlong)pvVar1 + (-8 - (longlong)pvVar2)))) {
LAB_140002ca0:
                    /* WARNING: Subroutine does not return */
      _invoke_watson((wchar_t *)0x0,(wchar_t *)0x0,(wchar_t *)0x0,0,0);
    }
    free(pvVar2);
  }
  param_1[2] = 0;
  param_1[3] = 7;
  *(undefined2 *)param_1 = 0;
  return;
}



// Function: FUN_140002cc0 @ 140002cc0
// Signature: ulonglong FUN_140002cc0(void);


/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

ulonglong FUN_140002cc0(void)

{
  longlong lVar1;
  code *pcVar2;
  undefined1 auVar3 [16];
  undefined1 auVar4 [16];
  undefined1 auVar5 [16];
  undefined1 auVar6 [16];
  undefined8 uVar7;
  BOOL BVar8;
  DWORD DVar9;
  LPWSTR pWVar10;
  longlong lVar11;
  void *pvVar12;
  LPCWSTR ***ppppWVar13;
  void *pvVar14;
  undefined1 (*pauVar15) [16];
  __uint64 _Var16;
  undefined1 auVar17 [8];
  LPCWSTR lpText;
  ulonglong uVar18;
  undefined2 *puVar19;
  undefined2 *puVar20;
  ulonglong uVar21;
  undefined1 *puVar22;
  short *_Src;
  size_t _Size;
  wchar_t *pwVar23;
  ulonglong uVar24;
  LPWSTR *ppWVar25;
  int iVar26;
  int local_1a8 [2];
  undefined1 local_1a0 [16];
  ulonglong uStack_190;
  ulonglong uStack_188;
  DWORD local_180 [2];
  undefined1 local_178 [8];
  undefined8 uStack_170;
  ulonglong uStack_168;
  ulonglong uStack_160;
  undefined1 local_158 [8];
  undefined8 uStack_150;
  undefined1 local_148 [16];
  undefined1 local_138 [16];
  undefined1 auStack_128 [16];
  LPCWSTR **local_118 [2];
  longlong local_108;
  ulonglong local_100;
  LPWSTR *local_f8;
  undefined1 local_f0 [16];
  undefined1 auStack_e0 [16];
  _PROCESS_INFORMATION local_d0 [2];
  short local_90;
  short local_8e [3];
  _STARTUPINFOW local_88;
  
  FUN_1400012a0((undefined1 (*) [16])local_118);
  if (local_108 == 0) {
    MessageBoxW((HWND)0x0,L"Could not determine the application\'s base directory.",
                L"Launcher Error",0x10);
    uVar24 = 1;
  }
  else {
    local_1a8[0] = 0;
    pWVar10 = GetCommandLineW();
    local_f8 = CommandLineToArgvW(pWVar10,local_1a8);
    uVar24 = 1;
    if ((local_f8 != (LPWSTR *)0x0) && (iVar26 = 1, ppWVar25 = local_f8, 1 < local_1a8[0])) {
      do {
        pWVar10 = ppWVar25[1];
        lVar11 = 0;
        while ((pWVar10[lVar11] == L"--complete-update"[lVar11] &&
               (pWVar10[lVar11 + 1] == L"--complete-update"[lVar11 + 1]))) {
          lVar11 = lVar11 + 2;
          if (lVar11 == 0x12) {
            FUN_140001c30(local_118);
            goto LAB_140002db7;
          }
        }
        iVar26 = iVar26 + 1;
        ppWVar25 = ppWVar25 + 1;
      } while (iVar26 < local_1a8[0]);
    }
LAB_140002db7:
    FUN_1400015a0(local_118);
    if (0x7ffffffffffffffeU - local_108 < 0x10) {
      FUN_1400011b0();
      pcVar2 = (code *)swi(3);
      uVar24 = (*pcVar2)();
      return uVar24;
    }
    uVar21 = local_108 + 0x10;
    uVar18 = 7;
    ppppWVar13 = local_118;
    if (7 < local_100) {
      ppppWVar13 = (LPCWSTR ***)local_118[0];
    }
    puVar22 = local_158;
    _local_158 = (undefined1  [16])0x0;
    uStack_150 = 0;
    local_148 = (undefined1  [16])0x0;
    if (7 < uVar21) {
      uVar18 = uVar21 | 7;
      if (uVar18 < 0x7fffffffffffffff) {
        if (uVar18 < 10) {
          uVar18 = 10;
        }
        if (0x7fffffffffffffff < uVar18 + 1) goto LAB_1400038c6;
        _Var16 = (uVar18 + 1) * 2;
        if (_Var16 != 0) goto LAB_140002e56;
        puVar22 = (undefined1 *)0x0;
      }
      else {
        uVar18 = 0x7ffffffffffffffe;
        _Var16 = 0xfffffffffffffffe;
LAB_140002e56:
        if (_Var16 < 0x1000) {
          puVar22 = (undefined1 *)operator_new(_Var16);
        }
        else {
          if (_Var16 + 0x27 <= _Var16) {
LAB_1400038c6:
            FUN_140001110();
            pcVar2 = (code *)swi(3);
            uVar24 = (*pcVar2)();
            return uVar24;
          }
          pvVar12 = operator_new(_Var16 + 0x27);
          if (pvVar12 == (void *)0x0) goto LAB_14000386f;
          puVar22 = (undefined1 *)((longlong)pvVar12 + 0x27U & 0xffffffffffffffe0);
          *(void **)(puVar22 + -8) = pvVar12;
        }
      }
      local_158 = (undefined1  [8])puVar22;
    }
    local_148._8_8_ = uVar18;
    local_148._0_8_ = uVar21;
    memcpy(puVar22,ppppWVar13,local_108 * 2);
    puVar19 = (undefined2 *)0x0;
    pwVar23 = (wchar_t *)(puVar22 + local_108 * 2);
    *(undefined8 *)pwVar23 = u__lib_Helios2_exe_14000ac40._0_8_;
    *(undefined8 *)(pwVar23 + 4) = u__lib_Helios2_exe_14000ac40._8_8_;
    *(undefined8 *)(pwVar23 + 8) = u__lib_Helios2_exe_14000ac40._16_8_;
    *(undefined8 *)(pwVar23 + 0xc) = u__lib_Helios2_exe_14000ac40._24_8_;
    *(undefined2 *)(puVar22 + uVar21 * 2) = 0;
    uVar7 = local_148._0_8_;
    if (local_148._0_8_ == 0x7ffffffffffffffe) {
      FUN_1400011b0();
      pcVar2 = (code *)swi(3);
      uVar24 = (*pcVar2)();
      return uVar24;
    }
    uVar21 = local_148._0_8_ + 1;
    uVar18 = 7;
    auVar17 = (undefined1  [8])local_158;
    if (7 < (ulonglong)local_148._8_8_) {
      auVar17 = local_158;
    }
    local_138 = (undefined1  [16])0x0;
    local_138._8_8_ = 0;
    auStack_128 = (undefined1  [16])0x0;
    puVar20 = (undefined2 *)local_138;
    if (7 < uVar21) {
      uVar18 = uVar21 | 7;
      if (uVar18 < 0x7fffffffffffffff) {
        if (uVar18 < 10) {
          uVar18 = 10;
        }
        if (0x7fffffffffffffff < uVar18 + 1) goto LAB_1400038cc;
        _Var16 = (uVar18 + 1) * 2;
        if (_Var16 != 0) goto LAB_140002f85;
      }
      else {
        uVar18 = 0x7ffffffffffffffe;
        _Var16 = 0xfffffffffffffffe;
LAB_140002f85:
        if (_Var16 < 0x1000) {
          puVar19 = (undefined2 *)operator_new(_Var16);
        }
        else {
          if (_Var16 + 0x27 <= _Var16) {
LAB_1400038cc:
            FUN_140001110();
            pcVar2 = (code *)swi(3);
            uVar24 = (*pcVar2)();
            return uVar24;
          }
          pvVar12 = operator_new(_Var16 + 0x27);
          if (pvVar12 == (void *)0x0) goto LAB_140003169;
          puVar19 = (undefined2 *)((longlong)pvVar12 + 0x27U & 0xffffffffffffffe0);
          *(void **)(puVar19 + -4) = pvVar12;
        }
      }
      local_138._0_8_ = puVar19;
      puVar20 = puVar19;
    }
    _Size = uVar7 * 2;
    auStack_128._8_8_ = uVar18;
    auStack_128._0_8_ = uVar21;
    *puVar20 = 0x22;
    memcpy(puVar20 + 1,(void *)auVar17,_Size);
    puVar20[uVar21] = 0;
    uVar7 = auStack_128._0_8_;
    if (auStack_128._8_8_ == auStack_128._0_8_) {
      pauVar15 = (undefined1 (*) [16])
                 FUN_140005d10((undefined8 *)local_138,1,_Size,&DAT_14000ac64,1);
    }
    else {
      auStack_128._0_8_ = auStack_128._0_8_ + 1;
      puVar22 = local_138;
      if (7 < (ulonglong)auStack_128._8_8_) {
        puVar22 = (undefined1 *)local_138._0_8_;
      }
      *(undefined4 *)(puVar22 + uVar7 * 2) = 0x22;
      pauVar15 = &local_138;
    }
    local_1a0 = *pauVar15;
    uStack_190 = *(ulonglong *)pauVar15[1];
    uStack_188 = *(ulonglong *)(pauVar15[1] + 8);
    *(undefined2 *)*pauVar15 = 0;
    *(undefined8 *)pauVar15[1] = 0;
    *(undefined8 *)(pauVar15[1] + 8) = 7;
    if (7 < (ulonglong)auStack_128._8_8_) {
      pvVar12 = (void *)local_138._0_8_;
      if ((0xfff < auStack_128._8_8_ * 2 + 2U) &&
         (pvVar12 = *(void **)(local_138._0_8_ + -8),
         0x1f < (ulonglong)(local_138._0_8_ + (-8 - (longlong)pvVar12)))) {
LAB_140003169:
                    /* WARNING: Subroutine does not return */
        _invoke_watson((wchar_t *)0x0,(wchar_t *)0x0,(wchar_t *)0x0,0,0);
      }
      free(pvVar12);
    }
    if (local_f8 != (LPWSTR *)0x0) {
      iVar26 = 1;
      ppWVar25 = local_f8;
      if (1 < local_1a8[0]) {
        do {
          uVar21 = uStack_190;
          lVar11 = 0;
          ppWVar25 = ppWVar25 + 1;
          pwVar23 = L"--complete-update";
LAB_140003140:
          if (((*ppWVar25)[lVar11] == L"--complete-update"[lVar11]) &&
             ((*ppWVar25)[lVar11 + 1] == L"--complete-update"[lVar11 + 1]))
          goto code_r0x000140003158;
          if (uStack_188 - uStack_190 < 2) {
            FUN_140005d10((undefined8 *)local_1a0,2,L"--complete-update",&DAT_14000ac68,2);
          }
          else {
            puVar22 = local_1a0;
            if (7 < uStack_188) {
              puVar22 = (undefined1 *)local_1a0._0_8_;
            }
            lVar11 = uStack_190 * 2;
            uStack_190 = uStack_190 + 2;
            *(undefined4 *)(puVar22 + lVar11) = 0x220020;
            *(undefined2 *)(puVar22 + uVar21 * 2 + 4) = 0;
          }
          uVar21 = uStack_190;
          pWVar10 = *ppWVar25;
          uVar18 = 0xffffffffffffffff;
          do {
            uVar18 = uVar18 + 1;
          } while (pWVar10[uVar18] != L'\0');
          if (uStack_188 - uStack_190 < uVar18) {
            FUN_140005d10((undefined8 *)local_1a0,uVar18,pwVar23,pWVar10,uVar18);
          }
          else {
            puVar22 = local_1a0;
            if (7 < uStack_188) {
              puVar22 = (undefined1 *)local_1a0._0_8_;
            }
            pwVar23 = (wchar_t *)(uVar18 * 2);
            lVar11 = uStack_190 * 2;
            uStack_190 = uVar18 + uStack_190;
            memmove(puVar22 + lVar11,pWVar10,(size_t)pwVar23);
            *(undefined2 *)(puVar22 + (uVar21 + uVar18) * 2) = 0;
          }
          if (uStack_188 == uStack_190) {
            FUN_140005d10((undefined8 *)local_1a0,1,pwVar23,&DAT_14000ac64,1);
          }
          else {
            puVar22 = local_1a0;
            if (7 < uStack_188) {
              puVar22 = (undefined1 *)local_1a0._0_8_;
            }
            lVar11 = uStack_190 * 2;
            uStack_190 = uStack_190 + 1;
            *(undefined4 *)(puVar22 + lVar11) = 0x22;
          }
LAB_1400032ad:
          iVar26 = iVar26 + 1;
          if (local_1a8[0] <= iVar26) break;
        } while( true );
      }
      LocalFree(local_f8);
    }
    pvVar12 = (void *)0x0;
    ppppWVar13 = local_118;
    if (7 < local_100) {
      ppppWVar13 = (LPCWSTR ***)local_118[0];
    }
    pWVar10 = (LPWSTR)local_1a0;
    if (7 < uStack_188) {
      pWVar10 = (LPWSTR)local_1a0._0_8_;
    }
    auVar17 = (undefined1  [8])local_158;
    if (7 < (ulonglong)local_148._8_8_) {
      auVar17 = local_158;
    }
    local_88.cb = 0x68;
    local_88.lpReserved = (LPWSTR)0x0;
    local_88.lpDesktop = (LPWSTR)0x0;
    local_88.lpTitle = (LPWSTR)0x0;
    local_88.dwX = 0;
    local_88.dwY = 0;
    local_88.dwXSize = 0;
    local_88.dwYSize = 0;
    local_88.dwXCountChars = 0;
    local_88.dwYCountChars = 0;
    local_88.dwFillAttribute = 0;
    local_88.dwFlags = 0;
    local_88.wShowWindow = 0;
    local_88.cbReserved2 = 0;
    local_88._68_4_ = 0;
    local_88.lpReserved2 = (LPBYTE)0x0;
    local_88.hStdInput = (HANDLE)0x0;
    local_88.hStdOutput = (HANDLE)0x0;
    local_88.hStdError = (HANDLE)0x0;
    BVar8 = CreateProcessW((LPCWSTR)auVar17,pWVar10,(LPSECURITY_ATTRIBUTES)0x0,
                           (LPSECURITY_ATTRIBUTES)0x0,0,0,(LPVOID)0x0,(LPCWSTR)ppppWVar13,&local_88,
                           local_d0);
    if (BVar8 == 0) {
      _Src = local_8e;
      DVar9 = GetLastError();
      uVar21 = (ulonglong)DVar9;
      do {
        _Src = _Src + -1;
        uVar18 = uVar21 / 10;
        *_Src = (short)uVar21 + (short)uVar18 * -10 + 0x30;
        uVar21 = uVar18;
      } while ((int)uVar18 != 0);
      _local_178 = (undefined1  [16])0x0;
      uStack_170 = 0;
      if (_Src == local_8e) {
        uStack_168 = _DAT_14000adb0;
        uStack_160 = _UNK_14000adb8;
        stack0xfffffffffffffe8a = SUB1614((undefined1  [16])0x0,2);
        auVar3._14_2_ = 0;
        auVar3._0_14_ = stack0xfffffffffffffe8a;
        _local_178 = auVar3 << 0x10;
      }
      else {
        uVar21 = (longlong)local_8e - (longlong)_Src >> 1;
        if (0x7ffffffffffffffe < uVar21) {
          FUN_1400011b0();
          pcVar2 = (code *)swi(3);
          uVar24 = (*pcVar2)();
          return uVar24;
        }
        uStack_168 = uVar21;
        if (uVar21 < 8) {
          uStack_160 = 7;
          memcpy(local_178,_Src,uVar21 * 2);
          *(undefined2 *)(local_178 + uVar21 * 2) = 0;
        }
        else {
          uVar18 = uVar21 | 7;
          if (uVar18 < 0x7fffffffffffffff) {
            if (uVar18 < 10) {
              uVar18 = 10;
            }
            if (0x7fffffffffffffff < uVar18 + 1) goto LAB_1400038b1;
            _Var16 = (uVar18 + 1) * 2;
            if (_Var16 != 0) goto LAB_1400034a4;
          }
          else {
            _Var16 = 0xfffffffffffffffe;
            uVar18 = 0x7ffffffffffffffe;
LAB_1400034a4:
            if (_Var16 < 0x1000) {
              pvVar12 = operator_new(_Var16);
            }
            else {
              if (_Var16 + 0x27 <= _Var16) {
LAB_1400038b1:
                FUN_140001110();
                pcVar2 = (code *)swi(3);
                uVar24 = (*pcVar2)();
                return uVar24;
              }
              pvVar14 = operator_new(_Var16 + 0x27);
              if (pvVar14 == (void *)0x0) {
                    /* WARNING: Subroutine does not return */
                _invoke_watson((wchar_t *)0x0,(wchar_t *)0x0,(wchar_t *)0x0,0,0);
              }
              pvVar12 = (void *)((longlong)pvVar14 + 0x27U & 0xffffffffffffffe0);
              *(void **)((longlong)pvVar12 - 8) = pvVar14;
            }
          }
          local_178 = (undefined1  [8])pvVar12;
          uStack_160 = uVar18;
          memcpy(pvVar12,_Src,uVar21 * 2);
          *(undefined2 *)(uVar21 * 2 + (longlong)pvVar12) = 0;
        }
      }
      if (uStack_160 - uStack_168 < 0x25) {
        pauVar15 = (undefined1 (*) [16])FUN_140005b60((undefined8 *)local_178,0x25);
      }
      else {
        auVar17 = (undefined1  [8])local_178;
        if (7 < uStack_160) {
          auVar17 = local_178;
        }
        if (((wchar_t *)((longlong)L"Failed to launch Helios2.exe. Error: " + 0x49) <
             (ulonglong)auVar17) ||
           ((wchar_t *)((longlong)auVar17 + uStack_168 * 2) <
            L"Failed to launch Helios2.exe. Error: ")) {
          lVar11 = 0x25;
        }
        else if (L"Failed to launch Helios2.exe. Error: " < (ulonglong)auVar17) {
          lVar11 = (longlong)((longlong)auVar17 + -0x14000ac80) >> 1;
        }
        else {
          lVar11 = 0;
        }
        lVar1 = uStack_168 * 2;
        uStack_168 = uStack_168 + 0x25;
        memmove((wchar_t *)((longlong)auVar17 + 0x4a),(void *)auVar17,lVar1 + 2);
        memcpy((void *)auVar17,L"Failed to launch Helios2.exe. Error: ",lVar11 * 2);
        memcpy((wchar_t *)((longlong)auVar17 + lVar11 * 2),
               L"Failed to launch Helios2.exe. Error: " + lVar11 + 0x25,(0x25 - lVar11) * 2);
        pauVar15 = (undefined1 (*) [16])local_178;
      }
      local_f0 = (undefined1  [16])0x0;
      auStack_e0 = (undefined1  [16])0x0;
      local_f0 = *pauVar15;
      auStack_e0 = pauVar15[1];
      *(undefined2 *)*pauVar15 = 0;
      *(undefined8 *)pauVar15[1] = 0;
      *(undefined8 *)(pauVar15[1] + 8) = 7;
      lpText = (LPCWSTR)local_f0;
      if (7 < (ulonglong)auStack_e0._8_8_) {
        lpText = (LPCWSTR)local_f0._0_8_;
      }
      MessageBoxW((HWND)0x0,lpText,L"Launcher Error",0x10);
      if (7 < (ulonglong)auStack_e0._8_8_) {
        pvVar12 = (void *)local_f0._0_8_;
        if ((0xfff < auStack_e0._8_8_ * 2 + 2U) &&
           (pvVar12 = *(void **)(local_f0._0_8_ + -8),
           0x1f < (ulonglong)(local_f0._0_8_ + (-8 - (longlong)pvVar12)))) goto LAB_140003706;
        free(pvVar12);
      }
      auVar4._14_2_ = 0;
      auVar4._0_14_ = local_f0._2_14_;
      local_f0 = auVar4 << 0x10;
      auStack_e0._8_8_ = _UNK_14000adb8;
      auStack_e0._0_8_ = _DAT_14000adb0;
      if (7 < uStack_160) {
        auVar17 = local_178;
        if ((0xfff < uStack_160 * 2 + 2) &&
           (auVar17 = *(undefined1 (*) [8])((longlong)local_178 + -8),
           0x1f < (ulonglong)((longlong)local_178 + (-8 - (longlong)auVar17)))) {
LAB_140003706:
                    /* WARNING: Subroutine does not return */
          _invoke_watson((wchar_t *)0x0,(wchar_t *)0x0,(wchar_t *)0x0,0,0);
        }
        free((void *)auVar17);
      }
    }
    else {
      WaitForSingleObject(local_d0[0].hProcess,0xffffffff);
      local_180[0] = 0;
      GetExitCodeProcess(local_d0[0].hProcess,local_180);
      CloseHandle(local_d0[0].hProcess);
      CloseHandle(local_d0[0].hThread);
      uVar24 = (ulonglong)local_180[0];
    }
    if (7 < uStack_188) {
      pvVar12 = (void *)local_1a0._0_8_;
      if ((0xfff < uStack_188 * 2 + 2) &&
         (pvVar12 = *(void **)(local_1a0._0_8_ + -8),
         0x1f < (ulonglong)(local_1a0._0_8_ + (-8 - (longlong)pvVar12)))) goto LAB_14000386f;
      free(pvVar12);
    }
    uStack_190 = 0;
    uStack_188 = 7;
    auVar5._14_2_ = 0;
    auVar5._0_14_ = local_1a0._2_14_;
    local_1a0 = auVar5 << 0x10;
    if (7 < (ulonglong)local_148._8_8_) {
      auVar17 = local_158;
      if ((0xfff < local_148._8_8_ * 2 + 2U) &&
         (auVar17 = *(undefined1 (*) [8])((longlong)local_158 + -8),
         0x1f < (ulonglong)((longlong)local_158 + (-8 - (longlong)auVar17)))) goto LAB_14000386f;
      free((void *)auVar17);
    }
    local_148._8_8_ = _UNK_14000adb8;
    local_148._0_8_ = _DAT_14000adb0;
    auVar6._14_2_ = 0;
    auVar6._0_14_ = stack0xfffffffffffffeaa;
    _local_158 = auVar6 << 0x10;
  }
  if (7 < local_100) {
    ppppWVar13 = (LPCWSTR ***)local_118[0];
    if ((0xfff < local_100 * 2 + 2) &&
       (ppppWVar13 = (LPCWSTR ***)local_118[0][-1],
       0x1f < (ulonglong)((longlong)local_118[0] + (-8 - (longlong)ppppWVar13)))) {
LAB_14000386f:
                    /* WARNING: Subroutine does not return */
      _invoke_watson((wchar_t *)0x0,(wchar_t *)0x0,(wchar_t *)0x0,0,0);
    }
    free(ppppWVar13);
  }
  return uVar24;
code_r0x000140003158:
  lVar11 = lVar11 + 2;
  if (lVar11 == 0x12) goto LAB_1400032ad;
  goto LAB_140003140;
}



// Function: FUN_1400038e0 @ 1400038e0
// Signature: void FUN_1400038e0(longlong *param_1);


void FUN_1400038e0(longlong *param_1)

{
  longlong lVar1;
  longlong lVar2;
  longlong lVar3;
  ulonglong uVar4;
  longlong lVar5;
  
  lVar1 = _Query_perf_frequency();
  lVar2 = _Query_perf_counter();
  if (lVar1 == 10000000) {
    lVar2 = lVar2 * 100;
  }
  else {
    if (lVar1 == 24000000) {
      lVar1 = lVar2 + SUB168(SEXT816(-0x4d0b03f86b6f730d) * SEXT816(lVar2),8);
      lVar3 = (lVar1 >> 0x18) - (lVar1 >> 0x3f);
      lVar1 = (lVar2 + lVar3 * -24000000) * 1000000000;
      lVar1 = lVar1 + SUB168(SEXT816(-0x4d0b03f86b6f730d) * SEXT816(lVar1),8);
      lVar2 = (lVar1 >> 0x18) - (lVar1 >> 0x3f);
    }
    else {
      lVar3 = lVar2 / lVar1;
      lVar2 = ((lVar2 % lVar1) * 1000000000) / lVar1;
    }
    lVar2 = lVar2 + lVar3 * 1000000000;
  }
  lVar1 = *param_1;
  if ((lVar1 != 0) && (-1 < lVar1)) {
    if (lVar2 < lVar1 * -1000000 + 0x7fffffffffffffff) {
      lVar2 = lVar2 + lVar1 * 1000000;
    }
    else {
      lVar2 = 0x7fffffffffffffff;
    }
  }
  while( true ) {
    lVar1 = _Query_perf_frequency();
    lVar3 = _Query_perf_counter();
    if (lVar1 == 10000000) {
      lVar3 = lVar3 * 100;
    }
    else {
      if (lVar1 == 24000000) {
        lVar1 = lVar3 + SUB168(SEXT816(-0x4d0b03f86b6f730d) * SEXT816(lVar3),8);
        lVar5 = (lVar1 >> 0x18) - (lVar1 >> 0x3f);
        lVar1 = (lVar3 + lVar5 * -24000000) * 1000000000;
        lVar1 = lVar1 + SUB168(SEXT816(-0x4d0b03f86b6f730d) * SEXT816(lVar1),8);
        lVar3 = (lVar1 >> 0x18) - (lVar1 >> 0x3f);
      }
      else {
        lVar5 = lVar3 / lVar1;
        lVar3 = ((lVar3 % lVar1) * 1000000000) / lVar1;
      }
      lVar3 = lVar3 + lVar5 * 1000000000;
    }
    if ((lVar2 == lVar3) || (lVar2 < lVar3)) break;
    lVar3 = lVar2 - lVar3;
    if ((lVar3 == 86400000000000) || (lVar3 < 86400000000000)) {
      uVar4 = lVar3 / 1000000;
      if ((longlong)(uVar4 * 1000000) < lVar3) {
        uVar4 = (ulonglong)((int)uVar4 + 1);
      }
      Sleep((DWORD)uVar4);
    }
    else {
      Sleep(86400000);
    }
  }
  return;
}



// Function: FUN_140003b60 @ 140003b60
// Signature: void FUN_140003b60(longlong *param_1);


void FUN_140003b60(longlong *param_1)

{
  void *pvVar1;
  void *_Memory;
  
  if (7 < (ulonglong)param_1[3]) {
    pvVar1 = (void *)*param_1;
    _Memory = pvVar1;
    if ((0xfff < param_1[3] * 2 + 2U) &&
       (_Memory = *(void **)((longlong)pvVar1 + -8),
       0x1f < (ulonglong)((longlong)pvVar1 + (-8 - (longlong)_Memory)))) {
                    /* WARNING: Subroutine does not return */
      _invoke_watson((wchar_t *)0x0,(wchar_t *)0x0,(wchar_t *)0x0,0,0);
    }
    free(_Memory);
  }
  param_1[3] = 7;
  param_1[2] = 0;
  *(undefined2 *)param_1 = 0;
  return;
}



// Function: FUN_140003be0 @ 140003be0
// Signature: void FUN_140003be0(longlong *param_1);


void FUN_140003be0(longlong *param_1)

{
  void *pvVar1;
  void *_Memory;
  
  if (0xf < (ulonglong)param_1[3]) {
    pvVar1 = (void *)*param_1;
    _Memory = pvVar1;
    if ((0xfff < param_1[3] + 1U) &&
       (_Memory = *(void **)((longlong)pvVar1 + -8),
       0x1f < (ulonglong)((longlong)pvVar1 + (-8 - (longlong)_Memory)))) {
                    /* WARNING: Subroutine does not return */
      _invoke_watson((wchar_t *)0x0,(wchar_t *)0x0,(wchar_t *)0x0,0,0);
    }
    free(_Memory);
  }
  param_1[2] = 0;
  param_1[3] = 0xf;
  *(undefined1 *)param_1 = 0;
  return;
}



// Function: FUN_140003c60 @ 140003c60
// Signature: void FUN_140003c60(basic_streambuf<char,struct_std::char_traits<char>_> *param_1,locale *param_2);


void FUN_140003c60(basic_streambuf<char,struct_std::char_traits<char>_> *param_1,locale *param_2)

{
  bool bVar1;
  facet *this;
  
  this = FUN_140005110(param_2);
  bVar1 = std::codecvt_base::always_noconv((codecvt_base *)this);
  if (bVar1) {
    *(undefined8 *)(param_1 + 0x68) = 0;
    return;
  }
  *(facet **)(param_1 + 0x68) = this;
                    /* WARNING: Could not recover jumptable at 0x000140003ca9. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  std::basic_streambuf<char,struct_std::char_traits<char>_>::_Init(param_1);
  return;
}



// Function: FUN_140003cb0 @ 140003cb0
// Signature: int FUN_140003cb0(longlong *param_1);


int FUN_140003cb0(longlong *param_1)

{
  int iVar1;
  
  if (param_1[0x10] != 0) {
    iVar1 = (**(code **)(*param_1 + 0x18))(param_1,0xffffffff);
    if (iVar1 != -1) {
      iVar1 = fflush((FILE *)param_1[0x10]);
      return (-1 < iVar1) - 1;
    }
  }
  return 0;
}



// Function: FUN_140003d00 @ 140003d00
// Signature: basic_streambuf<char,struct_std::char_traits<char>_> *FUN_140003d00(basic_streambuf<char,struct_std::char_traits<char>_> *param_1,char *param_2,size_t param_3);


basic_streambuf<char,struct_std::char_traits<char>_> *
FUN_140003d00(basic_streambuf<char,struct_std::char_traits<char>_> *param_1,char *param_2,
             size_t param_3)

{
  longlong lVar1;
  int iVar2;
  undefined8 local_res8;
  undefined8 local_res10;
  undefined8 local_res18 [2];
  
  if ((param_2 != (char *)0x0) || (iVar2 = 4, param_3 != 0)) {
    iVar2 = 0;
  }
  if (*(FILE **)(param_1 + 0x80) != (FILE *)0x0) {
    iVar2 = setvbuf(*(FILE **)(param_1 + 0x80),param_2,iVar2,param_3);
    if (iVar2 == 0) {
      lVar1 = *(longlong *)(param_1 + 0x80);
      param_1[0x7c] = (basic_streambuf<char,struct_std::char_traits<char>_>)0x1;
      param_1[0x71] = (basic_streambuf<char,struct_std::char_traits<char>_>)0x0;
      std::basic_streambuf<char,struct_std::char_traits<char>_>::_Init(param_1);
      if (lVar1 != 0) {
        local_res8 = 0;
        local_res10 = 0;
        local_res18[0] = 0;
        _get_stream_buffer_pointers(lVar1,&local_res8,&local_res10,local_res18);
        *(undefined8 *)(param_1 + 0x18) = local_res8;
        *(undefined8 *)(param_1 + 0x20) = local_res8;
        *(undefined8 *)(param_1 + 0x38) = local_res10;
        *(undefined8 *)(param_1 + 0x40) = local_res10;
        *(undefined8 *)(param_1 + 0x50) = local_res18[0];
        *(undefined8 *)(param_1 + 0x58) = local_res18[0];
      }
      *(undefined8 *)(param_1 + 0x74) = DAT_14000e938;
      *(longlong *)(param_1 + 0x80) = lVar1;
      *(undefined8 *)(param_1 + 0x68) = 0;
      return param_1;
    }
  }
  return (basic_streambuf<char,struct_std::char_traits<char>_> *)0x0;
}



// Function: FUN_140003de0 @ 140003de0
// Signature: longlong * FUN_140003de0(longlong *param_1,longlong *param_2,longlong *param_3);


longlong * FUN_140003de0(longlong *param_1,longlong *param_2,longlong *param_3)

{
  longlong lVar1;
  longlong lVar2;
  bool bVar3;
  int iVar4;
  longlong local_res8;
  
  local_res8 = param_3[1] + *param_3;
  if (param_1[0x10] != 0) {
    bVar3 = FUN_140005580(param_1);
    if (bVar3) {
      iVar4 = fsetpos((FILE *)param_1[0x10],&local_res8);
      if (iVar4 == 0) {
        *(longlong *)((longlong)param_1 + 0x74) = param_3[2];
        if (*(longlong **)param_1[3] == param_1 + 0xe) {
          lVar1 = param_1[0x11];
          lVar2 = param_1[0x12];
          *(longlong *)param_1[3] = lVar1;
          *(longlong *)param_1[7] = lVar1;
          *(int *)param_1[10] = (int)lVar2 - (int)lVar1;
        }
        lVar1 = *(longlong *)((longlong)param_1 + 0x74);
        *param_2 = local_res8;
        param_2[1] = 0;
        param_2[2] = lVar1;
        return param_2;
      }
    }
  }
  *param_2 = -1;
  param_2[1] = 0;
  param_2[2] = 0;
  return param_2;
}



// Function: FUN_140003ec0 @ 140003ec0
// Signature: fpos_t * FUN_140003ec0(longlong *param_1,fpos_t *param_2,longlong param_3,int param_4);


fpos_t * FUN_140003ec0(longlong *param_1,fpos_t *param_2,longlong param_3,int param_4)

{
  longlong lVar1;
  longlong lVar2;
  fpos_t fVar3;
  bool bVar4;
  int iVar5;
  fpos_t local_res8;
  
  if (((*(longlong **)param_1[7] == param_1 + 0xe) && (param_4 == 1)) && (param_1[0xd] == 0)) {
    param_3 = param_3 + -1;
  }
  if ((((param_1[0x10] != 0) && (bVar4 = FUN_140005580(param_1), bVar4)) &&
      (((param_3 == 0 && (param_4 == 1)) ||
       (iVar5 = _fseeki64((FILE *)param_1[0x10],param_3,param_4), iVar5 == 0)))) &&
     (iVar5 = fgetpos((FILE *)param_1[0x10],&local_res8), iVar5 == 0)) {
    if (*(longlong **)param_1[3] == param_1 + 0xe) {
      lVar1 = param_1[0x11];
      lVar2 = param_1[0x12];
      *(longlong *)param_1[3] = lVar1;
      *(longlong *)param_1[7] = lVar1;
      *(int *)param_1[10] = (int)lVar2 - (int)lVar1;
    }
    fVar3 = *(fpos_t *)((longlong)param_1 + 0x74);
    *param_2 = local_res8;
    param_2[2] = fVar3;
    param_2[1] = 0;
    return param_2;
  }
  *param_2 = -1;
  param_2[1] = 0;
  param_2[2] = 0;
  return param_2;
}



// Function: FUN_140003fc0 @ 140003fc0
// Signature: longlong FUN_140003fc0(basic_streambuf<char,struct_std::char_traits<char>_> *param_1,char *param_2,size_t param_3);


longlong FUN_140003fc0(basic_streambuf<char,struct_std::char_traits<char>_> *param_1,char *param_2,
                      size_t param_3)

{
  __int64 _Var1;
  size_t sVar2;
  int iVar3;
  size_t _Count;
  
  if (*(longlong *)(param_1 + 0x68) != 0) {
                    /* WARNING: Could not recover jumptable at 0x000140003fdf. Too many branches */
                    /* WARNING: Treating indirect jump as call */
    _Var1 = std::basic_streambuf<char,struct_std::char_traits<char>_>::xsputn
                      (param_1,param_2,param_3);
    return _Var1;
  }
  if ((void *)**(undefined8 **)(param_1 + 0x40) == (void *)0x0) {
    iVar3 = 0;
  }
  else {
    iVar3 = **(int **)(param_1 + 0x58);
  }
  _Count = param_3;
  if (0 < (longlong)param_3) {
    if (0 < iVar3) {
      sVar2 = (longlong)iVar3;
      if ((longlong)param_3 < (longlong)iVar3) {
        sVar2 = param_3;
      }
      memcpy((void *)**(undefined8 **)(param_1 + 0x40),param_2,sVar2);
      _Count = param_3 - sVar2;
      param_2 = param_2 + sVar2;
      **(int **)(param_1 + 0x58) = **(int **)(param_1 + 0x58) - (int)sVar2;
      **(longlong **)(param_1 + 0x40) = **(longlong **)(param_1 + 0x40) + (longlong)(int)sVar2;
      if ((longlong)_Count < 1) goto LAB_140004068;
    }
    if (*(FILE **)(param_1 + 0x80) != (FILE *)0x0) {
      sVar2 = fwrite(param_2,1,_Count,*(FILE **)(param_1 + 0x80));
      _Count = _Count - sVar2;
    }
  }
LAB_140004068:
  return param_3 - _Count;
}



// Function: FUN_140004090 @ 140004090
// Signature: longlong FUN_140004090(basic_streambuf<char,struct_std::char_traits<char>_> *param_1,char *param_2,ulonglong param_3);


longlong FUN_140004090(basic_streambuf<char,struct_std::char_traits<char>_> *param_1,char *param_2,
                      ulonglong param_3)

{
  longlong lVar1;
  undefined8 uVar2;
  __int64 _Var3;
  size_t sVar4;
  int iVar5;
  ulonglong _Size;
  ulonglong _Count;
  
  if ((longlong)param_3 < 1) {
    return 0;
  }
  if (*(longlong *)(param_1 + 0x68) != 0) {
                    /* WARNING: Could not recover jumptable at 0x0001400040d8. Too many branches */
                    /* WARNING: Treating indirect jump as call */
    _Var3 = std::basic_streambuf<char,struct_std::char_traits<char>_>::xsgetn
                      (param_1,param_2,param_3);
    return _Var3;
  }
  if ((void *)**(undefined8 **)(param_1 + 0x38) == (void *)0x0) {
    iVar5 = 0;
  }
  else {
    iVar5 = **(int **)(param_1 + 0x50);
  }
  _Count = param_3;
  if (iVar5 != 0) {
    _Size = param_3;
    if ((ulonglong)(longlong)iVar5 < param_3) {
      _Size = (longlong)iVar5;
    }
    memcpy(param_2,(void *)**(undefined8 **)(param_1 + 0x38),_Size);
    param_2 = param_2 + _Size;
    _Count = param_3 - _Size;
    **(int **)(param_1 + 0x50) = **(int **)(param_1 + 0x50) - (int)_Size;
    **(longlong **)(param_1 + 0x38) = **(longlong **)(param_1 + 0x38) + (longlong)(int)_Size;
  }
  if (*(longlong *)(param_1 + 0x80) != 0) {
    if ((basic_streambuf<char,struct_std::char_traits<char>_> *)**(longlong **)(param_1 + 0x18) ==
        param_1 + 0x70) {
      lVar1 = *(longlong *)(param_1 + 0x88);
      uVar2 = *(undefined8 *)(param_1 + 0x90);
      **(longlong **)(param_1 + 0x18) = lVar1;
      **(longlong **)(param_1 + 0x38) = lVar1;
      **(int **)(param_1 + 0x50) = (int)uVar2 - (int)lVar1;
    }
    do {
      if (_Count < 0x1000) {
        if (_Count != 0) {
          sVar4 = fread(param_2,1,_Count,*(FILE **)(param_1 + 0x80));
          _Count = _Count - sVar4;
        }
        break;
      }
      sVar4 = fread(param_2,1,0xfff,*(FILE **)(param_1 + 0x80));
      param_2 = param_2 + sVar4;
      _Count = _Count - sVar4;
    } while (sVar4 == 0xfff);
  }
  return param_3 - _Count;
}



// Function: FUN_1400041f0 @ 1400041f0
// Signature: ulonglong FUN_1400041f0(longlong param_1);


ulonglong FUN_1400041f0(longlong param_1)

{
  ulonglong uVar1;
  ulonglong uVar2;
  byte *pbVar3;
  undefined8 uVar4;
  longlong lVar5;
  code *pcVar6;
  undefined1 auVar7 [16];
  uint uVar8;
  int iVar9;
  ulonglong uVar10;
  undefined1 *puVar11;
  __uint64 _Var12;
  void *pvVar13;
  FILE *_File;
  char *pcVar14;
  uint uVar15;
  size_t sVar16;
  char *pcVar17;
  ulonglong uVar18;
  char *pcVar19;
  char cVar20;
  char *local_res8;
  byte *local_res10;
  byte local_68;
  char local_67 [7];
  undefined1 local_60 [16];
  size_t local_50;
  ulonglong local_48;
  
  uVar10 = **(ulonglong **)(param_1 + 0x38);
  if (uVar10 != 0) {
    iVar9 = **(int **)(param_1 + 0x50);
    if (uVar10 < uVar10 + (longlong)iVar9) {
      **(int **)(param_1 + 0x50) = iVar9 + -1;
      pbVar3 = (byte *)**(longlong **)(param_1 + 0x38);
      **(longlong **)(param_1 + 0x38) = (longlong)(pbVar3 + 1);
      return (ulonglong)*pbVar3;
    }
  }
  if (*(longlong *)(param_1 + 0x80) == 0) {
    uVar10 = 0xffffffff;
  }
  else {
    if (**(longlong **)(param_1 + 0x18) == param_1 + 0x70) {
      uVar4 = *(undefined8 *)(param_1 + 0x90);
      lVar5 = *(longlong *)(param_1 + 0x88);
      **(longlong **)(param_1 + 0x18) = lVar5;
      **(longlong **)(param_1 + 0x38) = lVar5;
      **(int **)(param_1 + 0x50) = (int)uVar4 - (int)lVar5;
    }
    _File = *(FILE **)(param_1 + 0x80);
    if (*(longlong *)(param_1 + 0x68) == 0) {
      uVar8 = fgetc(_File);
      uVar15 = 0xffffffff;
      if (uVar8 != 0xffffffff) {
        uVar15 = uVar8 & 0xff;
      }
    }
    else {
      local_50 = 0;
      local_48 = 0xf;
      local_60._1_15_ = SUB1615((undefined1  [16])0x0,1);
      auVar7[0xf] = 0;
      auVar7._0_15_ = local_60._1_15_;
      local_60 = auVar7 << 8;
LAB_1400042ca:
      iVar9 = fgetc(_File);
      uVar10 = local_48;
      sVar16 = local_50;
      if (iVar9 != -1) {
        cVar20 = (char)iVar9;
        if (local_50 < local_48) {
          puVar11 = local_60;
          if (0xf < local_48) {
            puVar11 = (undefined1 *)local_60._0_8_;
          }
          pcVar17 = puVar11 + local_50;
          local_50 = local_50 + 1;
          *pcVar17 = cVar20;
          puVar11[sVar16 + 1] = 0;
          pcVar17 = (char *)local_60._0_8_;
        }
        else {
          if (local_50 == 0x7fffffffffffffff) {
            FUN_1400011b0();
LAB_1400045cb:
            FUN_140001110();
            pcVar6 = (code *)swi(3);
            uVar10 = (*pcVar6)();
            return uVar10;
          }
          uVar1 = local_50 + 1;
          uVar18 = uVar1 | 0xf;
          if (uVar18 < 0x8000000000000000) {
            if (0x7fffffffffffffff - (local_48 >> 1) < local_48) {
              uVar18 = 0x7fffffffffffffff;
              _Var12 = 0x8000000000000027;
              goto LAB_140004395;
            }
            uVar2 = local_48 + (local_48 >> 1);
            if (uVar18 < uVar2) {
              uVar18 = uVar2;
            }
            uVar2 = uVar18 + 1;
            if (uVar2 == 0) {
              pcVar17 = (char *)0x0;
            }
            else {
              if (0xfff < uVar2) {
                _Var12 = uVar18 + 0x28;
                if (_Var12 <= uVar2) goto LAB_1400045cb;
                goto LAB_140004395;
              }
              pcVar17 = (char *)operator_new(uVar2);
            }
          }
          else {
            uVar18 = 0x7fffffffffffffff;
            _Var12 = 0x8000000000000027;
LAB_140004395:
            pvVar13 = operator_new(_Var12);
            if (pvVar13 == (void *)0x0) goto LAB_1400044dd;
            pcVar17 = (char *)((longlong)pvVar13 + 0x27U & 0xffffffffffffffe0);
            *(void **)(pcVar17 + -8) = pvVar13;
          }
          local_50 = uVar1;
          local_48 = uVar18;
          if (uVar10 < 0x10) {
            memcpy(pcVar17,local_60,sVar16);
            pcVar17[sVar16] = cVar20;
            pcVar17[sVar16 + 1] = '\0';
          }
          else {
            uVar4 = local_60._0_8_;
            memcpy(pcVar17,(void *)local_60._0_8_,sVar16);
            pcVar17[sVar16] = cVar20;
            pcVar17[sVar16 + 1] = '\0';
            pvVar13 = (void *)uVar4;
            if ((0xfff < uVar10 + 1) &&
               (pvVar13 = *(void **)(uVar4 + -8),
               0x1f < (ulonglong)(uVar4 + (-8 - (longlong)pvVar13)))) {
LAB_1400044dd:
                    /* WARNING: Subroutine does not return */
              _invoke_watson((wchar_t *)0x0,(wchar_t *)0x0,(wchar_t *)0x0,0,0);
            }
            free(pvVar13);
          }
          local_60._0_8_ = pcVar17;
        }
        pcVar14 = local_60;
        if (0xf < local_48) {
          pcVar14 = pcVar17;
        }
        pcVar19 = local_60;
        if (0xf < local_48) {
          pcVar19 = pcVar17;
        }
        iVar9 = std::codecvt<char,char,struct__Mbstatet>::in
                          (*(codecvt<char,char,struct__Mbstatet> **)(param_1 + 0x68),
                           (_Mbstatet *)(param_1 + 0x74),pcVar19,pcVar14 + local_50,&local_res8,
                           (char *)&local_68,local_67,(char **)&local_res10);
        if ((iVar9 == 0) || (iVar9 == 1)) {
          if (local_res10 == &local_68) goto code_r0x000140004497;
          pcVar17 = local_60;
          if (0xf < local_48) {
            pcVar17 = (char *)local_60._0_8_;
          }
          pcVar17 = pcVar17 + (local_50 - (longlong)local_res8);
          while (0 < (longlong)pcVar17) {
            pcVar17 = pcVar17 + -1;
            ungetc((int)pcVar17[(longlong)local_res8],*(FILE **)(param_1 + 0x80));
          }
          uVar15 = (uint)local_68;
          goto LAB_14000455a;
        }
        if (iVar9 == 3) {
          pcVar17 = local_60;
          if (0xf < local_48) {
            pcVar17 = (char *)local_60._0_8_;
          }
          uVar15 = (uint)*pcVar17;
          goto LAB_14000455a;
        }
      }
      uVar15 = 0xffffffff;
LAB_14000455a:
      if (0xf < local_48) {
        pvVar13 = (void *)local_60._0_8_;
        if ((0xfff < local_48 + 1) &&
           (pvVar13 = *(void **)(local_60._0_8_ + -8),
           0x1f < (ulonglong)(local_60._0_8_ + (-8 - (longlong)pvVar13)))) {
                    /* WARNING: Subroutine does not return */
          _invoke_watson((wchar_t *)0x0,(wchar_t *)0x0,(wchar_t *)0x0,0,0);
        }
        free(pvVar13);
      }
    }
    uVar10 = (ulonglong)uVar15;
  }
  return uVar10;
code_r0x000140004497:
  pcVar17 = local_60;
  if (0xf < local_48) {
    pcVar17 = (char *)local_60._0_8_;
  }
  uVar10 = (longlong)local_res8 - (longlong)pcVar17;
  if (local_50 < (ulonglong)((longlong)local_res8 - (longlong)pcVar17)) {
    uVar10 = local_50;
  }
  pcVar17 = local_60;
  if (0xf < local_48) {
    pcVar17 = (char *)local_60._0_8_;
  }
  sVar16 = local_50 - uVar10;
  memmove(pcVar17,pcVar17 + uVar10,sVar16 + 1);
  _File = *(FILE **)(param_1 + 0x80);
  local_50 = sVar16;
  goto LAB_1400042ca;
}



// Function: FUN_1400045e0 @ 1400045e0
// Signature: ulonglong FUN_1400045e0(longlong *param_1);


ulonglong FUN_1400045e0(longlong *param_1)

{
  byte *pbVar1;
  ulonglong uVar2;
  
  pbVar1 = *(byte **)param_1[7];
  if ((pbVar1 != (byte *)0x0) && (pbVar1 < pbVar1 + *(int *)param_1[10])) {
    return (ulonglong)*pbVar1;
  }
  uVar2 = (**(code **)(*param_1 + 0x38))(param_1);
  if ((int)uVar2 == -1) {
    return uVar2;
  }
  (**(code **)(*param_1 + 0x20))(param_1,uVar2 & 0xffffffff);
  return uVar2 & 0xffffffff;
}



// Function: FUN_140004650 @ 140004650
// Signature: uint FUN_140004650(longlong param_1,uint param_2);


uint FUN_140004650(longlong param_1,uint param_2)

{
  undefined1 *puVar1;
  ulonglong uVar2;
  undefined1 *puVar3;
  int iVar4;
  
  uVar2 = **(ulonglong **)(param_1 + 0x38);
  if (((uVar2 != 0) && (**(ulonglong **)(param_1 + 0x18) < uVar2)) &&
     ((param_2 == 0xffffffff || (*(byte *)(uVar2 - 1) == param_2)))) {
    **(int **)(param_1 + 0x50) = **(int **)(param_1 + 0x50) + 1;
    **(longlong **)(param_1 + 0x38) = **(longlong **)(param_1 + 0x38) + -1;
    if (param_2 == 0xffffffff) {
      param_2 = 0;
    }
    return param_2;
  }
  if ((*(FILE **)(param_1 + 0x80) != (FILE *)0x0) && (param_2 != 0xffffffff)) {
    if ((*(longlong *)(param_1 + 0x68) == 0) &&
       (iVar4 = ungetc(param_2 & 0xff,*(FILE **)(param_1 + 0x80)), iVar4 != -1)) {
      return param_2;
    }
    puVar1 = (undefined1 *)(param_1 + 0x70);
    if ((undefined1 *)**(longlong **)(param_1 + 0x38) != puVar1) {
      *puVar1 = (char)param_2;
      puVar3 = (undefined1 *)**(longlong **)(param_1 + 0x18);
      if (puVar3 != puVar1) {
        *(undefined1 **)(param_1 + 0x88) = puVar3;
        *(longlong *)(param_1 + 0x90) =
             (longlong)**(int **)(param_1 + 0x50) + **(longlong **)(param_1 + 0x38);
      }
      **(longlong **)(param_1 + 0x18) = (longlong)puVar1;
      **(longlong **)(param_1 + 0x38) = (longlong)puVar1;
      **(int **)(param_1 + 0x50) = ((int)param_1 - (int)puVar1) + 0x71;
      return param_2;
    }
  }
  return 0xffffffff;
}



// Function: FUN_140004740 @ 140004740
// Signature: int FUN_140004740(longlong param_1,int param_2);


int FUN_140004740(longlong param_1,int param_2)

{
  ulonglong uVar1;
  char *pcVar2;
  longlong lVar3;
  undefined8 uVar4;
  int iVar5;
  size_t sVar6;
  size_t _Count;
  char *local_res18;
  char *local_res20;
  char local_38;
  char local_37 [7];
  char local_30 [32];
  char local_10 [8];
  
  if (param_2 == -1) {
    return 0;
  }
  uVar1 = **(ulonglong **)(param_1 + 0x40);
  local_38 = (char)param_2;
  if (uVar1 != 0) {
    iVar5 = **(int **)(param_1 + 0x58);
    if (uVar1 < uVar1 + (longlong)iVar5) {
      **(int **)(param_1 + 0x58) = iVar5 + -1;
      pcVar2 = (char *)**(longlong **)(param_1 + 0x40);
      **(longlong **)(param_1 + 0x40) = (longlong)(pcVar2 + 1);
      *pcVar2 = local_38;
      return param_2;
    }
  }
  if (*(longlong *)(param_1 + 0x80) != 0) {
    if (**(longlong **)(param_1 + 0x18) == param_1 + 0x70) {
      lVar3 = *(longlong *)(param_1 + 0x88);
      uVar4 = *(undefined8 *)(param_1 + 0x90);
      **(longlong **)(param_1 + 0x18) = lVar3;
      **(longlong **)(param_1 + 0x38) = lVar3;
      **(int **)(param_1 + 0x50) = (int)uVar4 - (int)lVar3;
    }
    if (*(codecvt<char,char,struct__Mbstatet> **)(param_1 + 0x68) ==
        (codecvt<char,char,struct__Mbstatet> *)0x0) {
      iVar5 = fputc((int)local_38,*(FILE **)(param_1 + 0x80));
      if (iVar5 != -1) {
        return param_2;
      }
      return -1;
    }
    iVar5 = std::codecvt<char,char,struct__Mbstatet>::out
                      (*(codecvt<char,char,struct__Mbstatet> **)(param_1 + 0x68),
                       (_Mbstatet *)(param_1 + 0x74),&local_38,local_37,&local_res20,local_30,
                       local_10,&local_res18);
    if ((iVar5 == 0) || (iVar5 == 1)) {
      _Count = (longlong)local_res18 - (longlong)local_30;
      if ((_Count == 0) ||
         (sVar6 = fwrite(local_30,1,_Count,*(FILE **)(param_1 + 0x80)), _Count == sVar6)) {
        *(undefined1 *)(param_1 + 0x71) = 1;
        if (local_res20 == &local_38) {
          return -1;
        }
        return param_2;
      }
    }
    else if (iVar5 == 3) {
      iVar5 = fputc((int)local_38,*(FILE **)(param_1 + 0x80));
      if (iVar5 != -1) {
        return param_2;
      }
      return -1;
    }
  }
  return -1;
}



// Function: FUN_140004900 @ 140004900
// Signature: void FUN_140004900(longlong param_1);


void FUN_140004900(longlong param_1)

{
  if (*(longlong *)(param_1 + 0x80) != 0) {
                    /* WARNING: Could not recover jumptable at 0x00014000490c. Too many branches */
                    /* WARNING: Treating indirect jump as call */
    _unlock_file();
    return;
  }
  return;
}



// Function: FUN_140004920 @ 140004920
// Signature: void FUN_140004920(longlong param_1);


void FUN_140004920(longlong param_1)

{
  if (*(longlong *)(param_1 + 0x80) != 0) {
                    /* WARNING: Could not recover jumptable at 0x00014000492c. Too many branches */
                    /* WARNING: Treating indirect jump as call */
    _lock_file();
    return;
  }
  return;
}



// Function: FUN_140004940 @ 140004940
// Signature: void FUN_140004940(basic_streambuf<char,struct_std::char_traits<char>_> *param_1);


void FUN_140004940(basic_streambuf<char,struct_std::char_traits<char>_> *param_1)

{
  undefined8 uVar1;
  longlong lVar2;
  
  *(undefined ***)param_1 = std::basic_filebuf<char,struct_std::char_traits<char>_>::vftable;
  if ((*(longlong *)(param_1 + 0x80) != 0) &&
     ((basic_streambuf<char,struct_std::char_traits<char>_> *)**(longlong **)(param_1 + 0x18) ==
      param_1 + 0x70)) {
    uVar1 = *(undefined8 *)(param_1 + 0x90);
    lVar2 = *(longlong *)(param_1 + 0x88);
    **(longlong **)(param_1 + 0x18) = lVar2;
    **(longlong **)(param_1 + 0x38) = lVar2;
    **(int **)(param_1 + 0x50) = (int)uVar1 - (int)lVar2;
  }
  if (param_1[0x7c] != (basic_streambuf<char,struct_std::char_traits<char>_>)0x0) {
    if (*(longlong *)(param_1 + 0x80) != 0) {
      if ((basic_streambuf<char,struct_std::char_traits<char>_> *)**(longlong **)(param_1 + 0x18) ==
          param_1 + 0x70) {
        uVar1 = *(undefined8 *)(param_1 + 0x90);
        lVar2 = *(longlong *)(param_1 + 0x88);
        **(longlong **)(param_1 + 0x18) = lVar2;
        **(longlong **)(param_1 + 0x38) = lVar2;
        **(int **)(param_1 + 0x50) = (int)uVar1 - (int)lVar2;
      }
      FUN_140005580((longlong *)param_1);
      fclose(*(FILE **)(param_1 + 0x80));
    }
    param_1[0x7c] = (basic_streambuf<char,struct_std::char_traits<char>_>)0x0;
    param_1[0x71] = (basic_streambuf<char,struct_std::char_traits<char>_>)0x0;
    std::basic_streambuf<char,struct_std::char_traits<char>_>::_Init(param_1);
    *(undefined8 *)(param_1 + 0x80) = 0;
    *(undefined8 *)(param_1 + 0x74) = DAT_14000e938;
    *(undefined8 *)(param_1 + 0x68) = 0;
  }
                    /* WARNING: Could not recover jumptable at 0x000140004a0d. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  std::basic_streambuf<char,struct_std::char_traits<char>_>::
  ~basic_streambuf<char,struct_std::char_traits<char>_>(param_1);
  return;
}



// Function: FUN_140004a20 @ 140004a20
// Signature: longlong * FUN_140004a20(longlong param_1,uint param_2);


longlong * FUN_140004a20(longlong param_1,uint param_2)

{
  longlong *_Memory;
  
  _Memory = (longlong *)(param_1 + -0xb0);
  FUN_140002ac0(_Memory);
  if ((param_2 & 1) != 0) {
    free(_Memory);
  }
  return _Memory;
}



// Function: FUN_140004a60 @ 140004a60
// Signature: basic_streambuf<char,struct_std::char_traits<char>_> *FUN_140004a60(basic_streambuf<char,struct_std::char_traits<char>_> *param_1,uint param_2);


basic_streambuf<char,struct_std::char_traits<char>_> *
FUN_140004a60(basic_streambuf<char,struct_std::char_traits<char>_> *param_1,uint param_2)

{
  undefined8 uVar1;
  longlong lVar2;
  
  *(undefined ***)param_1 = std::basic_filebuf<char,struct_std::char_traits<char>_>::vftable;
  if ((*(longlong *)(param_1 + 0x80) != 0) &&
     ((basic_streambuf<char,struct_std::char_traits<char>_> *)**(longlong **)(param_1 + 0x18) ==
      param_1 + 0x70)) {
    uVar1 = *(undefined8 *)(param_1 + 0x90);
    lVar2 = *(longlong *)(param_1 + 0x88);
    **(longlong **)(param_1 + 0x18) = lVar2;
    **(longlong **)(param_1 + 0x38) = lVar2;
    **(int **)(param_1 + 0x50) = (int)uVar1 - (int)lVar2;
  }
  if (param_1[0x7c] != (basic_streambuf<char,struct_std::char_traits<char>_>)0x0) {
    if (*(longlong *)(param_1 + 0x80) != 0) {
      if ((basic_streambuf<char,struct_std::char_traits<char>_> *)**(longlong **)(param_1 + 0x18) ==
          param_1 + 0x70) {
        uVar1 = *(undefined8 *)(param_1 + 0x90);
        lVar2 = *(longlong *)(param_1 + 0x88);
        **(longlong **)(param_1 + 0x18) = lVar2;
        **(longlong **)(param_1 + 0x38) = lVar2;
        **(int **)(param_1 + 0x50) = (int)uVar1 - (int)lVar2;
      }
      FUN_140005580((longlong *)param_1);
      fclose(*(FILE **)(param_1 + 0x80));
    }
    param_1[0x7c] = (basic_streambuf<char,struct_std::char_traits<char>_>)0x0;
    param_1[0x71] = (basic_streambuf<char,struct_std::char_traits<char>_>)0x0;
    std::basic_streambuf<char,struct_std::char_traits<char>_>::_Init(param_1);
    *(undefined8 *)(param_1 + 0x80) = 0;
    *(undefined8 *)(param_1 + 0x74) = DAT_14000e938;
    *(undefined8 *)(param_1 + 0x68) = 0;
  }
  std::basic_streambuf<char,struct_std::char_traits<char>_>::
  ~basic_streambuf<char,struct_std::char_traits<char>_>(param_1);
  if ((param_2 & 1) != 0) {
    free(param_1);
  }
  return param_1;
}



// Function: FUN_140004b60 @ 140004b60
// Signature: basic_istream<char,struct_std::char_traits<char>_> *FUN_140004b60(basic_istream<char,struct_std::char_traits<char>_> *param_1,longlong *param_2,undefined8 param_3);


basic_istream<char,struct_std::char_traits<char>_> *
FUN_140004b60(basic_istream<char,struct_std::char_traits<char>_> *param_1,longlong *param_2,
             undefined8 param_3)

{
  ulonglong uVar1;
  ulonglong uVar2;
  uint uVar3;
  bool bVar4;
  bool bVar5;
  uint uVar6;
  longlong *plVar7;
  
  uVar3 = (uint)param_3;
  uVar6 = 0;
  bVar4 = false;
  if (*(longlong **)(param_1 + (longlong)*(int *)(*(longlong *)param_1 + 4) + 0x48) !=
      (longlong *)0x0) {
    (**(code **)(**(longlong **)(param_1 + (longlong)*(int *)(*(longlong *)param_1 + 4) + 0x48) + 8)
    )();
  }
  bVar5 = std::basic_istream<char,struct_std::char_traits<char>_>::_Ipfx(param_1,true);
  if (bVar5) {
    param_2[2] = 0;
    plVar7 = param_2;
    if (0xf < (ulonglong)param_2[3]) {
      plVar7 = (longlong *)*param_2;
    }
    *(undefined1 *)plVar7 = 0;
    uVar6 = std::basic_streambuf<char,struct_std::char_traits<char>_>::sgetc
                      (*(basic_streambuf<char,struct_std::char_traits<char>_> **)
                        (param_1 + (longlong)*(int *)(*(longlong *)param_1 + 4) + 0x48));
    while (uVar6 != 0xffffffff) {
      if (uVar6 == (uVar3 & 0xff)) {
        bVar4 = true;
        std::basic_streambuf<char,struct_std::char_traits<char>_>::sbumpc
                  (*(basic_streambuf<char,struct_std::char_traits<char>_> **)
                    (param_1 + (longlong)*(int *)(*(longlong *)param_1 + 4) + 0x48));
        uVar6 = 0;
        goto LAB_140004c44;
      }
      uVar1 = param_2[2];
      if (0x7ffffffffffffffe < uVar1) {
        uVar6 = 2;
        goto LAB_140004c44;
      }
      uVar2 = param_2[3];
      if (uVar1 < uVar2) {
        param_2[2] = uVar1 + 1;
        plVar7 = param_2;
        if (0xf < uVar2) {
          plVar7 = (longlong *)*param_2;
        }
        *(char *)(uVar1 + (longlong)plVar7) = (char)uVar6;
        *(undefined1 *)(uVar1 + 1 + (longlong)plVar7) = 0;
      }
      else {
        FUN_140004f90(param_2,uVar2,param_3,(char)uVar6);
      }
      bVar4 = true;
      uVar6 = std::basic_streambuf<char,struct_std::char_traits<char>_>::snextc
                        (*(basic_streambuf<char,struct_std::char_traits<char>_> **)
                          (param_1 + (longlong)*(int *)(*(longlong *)param_1 + 4) + 0x48));
    }
    uVar6 = 1;
LAB_140004c44:
    if (bVar4) goto LAB_140004cba;
  }
  uVar6 = uVar6 | 2;
LAB_140004cba:
  std::basic_ios<char,struct_std::char_traits<char>_>::setstate
            ((basic_ios<char,struct_std::char_traits<char>_> *)
             (param_1 + *(int *)(*(longlong *)param_1 + 4)),uVar6,false);
  if (*(longlong **)(param_1 + (longlong)*(int *)(*(longlong *)param_1 + 4) + 0x48) !=
      (longlong *)0x0) {
    (**(code **)(**(longlong **)(param_1 + (longlong)*(int *)(*(longlong *)param_1 + 4) + 0x48) +
                0x10))();
  }
  return param_1;
}



// Function: FUN_140004d10 @ 140004d10
// Signature: void FUN_140004d10(undefined8 *param_1,undefined8 param_2,ulonglong param_3);


void FUN_140004d10(undefined8 *param_1,undefined8 param_2,ulonglong param_3)

{
  code *pcVar1;
  ulonglong uVar2;
  void *pvVar3;
  __uint64 _Var4;
  undefined2 *puVar5;
  undefined8 *puVar6;
  undefined2 *puVar7;
  
  if (0x7ffffffffffffffe < param_3) {
    FUN_1400011b0();
    pcVar1 = (code *)swi(3);
    (*pcVar1)();
    return;
  }
  puVar5 = (undefined2 *)0x0;
  if (param_3 < 8) {
    param_1[2] = param_3;
    param_1[3] = 7;
    uVar2 = param_3;
    puVar6 = param_1;
    if (param_3 != 0) {
      for (; uVar2 != 0; uVar2 = uVar2 - 1) {
        *(undefined2 *)puVar6 = 0;
        puVar6 = (undefined8 *)((longlong)puVar6 + 2);
      }
    }
    *(undefined2 *)((longlong)param_1 + param_3 * 2) = 0;
    return;
  }
  uVar2 = param_3 | 7;
  if (uVar2 < 0x7fffffffffffffff) {
    if (uVar2 < 10) {
      uVar2 = 10;
    }
    if (0x7fffffffffffffff < uVar2 + 1) goto LAB_140004e44;
    _Var4 = (uVar2 + 1) * 2;
    if (_Var4 == 0) goto LAB_140004e17;
  }
  else {
    _Var4 = 0xfffffffffffffffe;
    uVar2 = 0x7ffffffffffffffe;
  }
  if (_Var4 < 0x1000) {
    puVar5 = (undefined2 *)operator_new(_Var4);
  }
  else {
    if (_Var4 + 0x27 <= _Var4) {
LAB_140004e44:
      FUN_140001110();
      pcVar1 = (code *)swi(3);
      (*pcVar1)();
      return;
    }
    pvVar3 = operator_new(_Var4 + 0x27);
    if (pvVar3 == (void *)0x0) {
                    /* WARNING: Subroutine does not return */
      _invoke_watson((wchar_t *)0x0,(wchar_t *)0x0,(wchar_t *)0x0,0,0);
    }
    puVar5 = (undefined2 *)((longlong)pvVar3 + 0x27U & 0xffffffffffffffe0);
    *(void **)(puVar5 + -4) = pvVar3;
  }
LAB_140004e17:
  *param_1 = puVar5;
  param_1[2] = param_3;
  param_1[3] = uVar2;
  puVar7 = puVar5;
  for (uVar2 = param_3; uVar2 != 0; uVar2 = uVar2 - 1) {
    *puVar7 = 0;
    puVar7 = puVar7 + 1;
  }
  puVar5[param_3] = 0;
  return;
}



// Function: FUN_140004e50 @ 140004e50
// Signature: void FUN_140004e50(undefined8 *param_1,void *param_2,ulonglong param_3);


void FUN_140004e50(undefined8 *param_1,void *param_2,ulonglong param_3)

{
  code *pcVar1;
  ulonglong uVar2;
  void *pvVar3;
  __uint64 _Var4;
  void *_Dst;
  
  if (0x7ffffffffffffffe < param_3) {
    FUN_1400011b0();
    pcVar1 = (code *)swi(3);
    (*pcVar1)();
    return;
  }
  if (param_3 < 8) {
    param_1[2] = param_3;
    param_1[3] = 7;
    memcpy(param_1,param_2,param_3 * 2);
    *(undefined2 *)(param_3 * 2 + (longlong)param_1) = 0;
    return;
  }
  uVar2 = param_3 | 7;
  _Dst = (void *)0x0;
  if (uVar2 < 0x7fffffffffffffff) {
    if (uVar2 < 10) {
      uVar2 = 10;
    }
    if (0x7fffffffffffffff < uVar2 + 1) goto LAB_140004f82;
    _Var4 = (uVar2 + 1) * 2;
    if (_Var4 == 0) goto LAB_140004f43;
  }
  else {
    _Var4 = 0xfffffffffffffffe;
    uVar2 = 0x7ffffffffffffffe;
  }
  if (_Var4 < 0x1000) {
    _Dst = operator_new(_Var4);
  }
  else {
    if (_Var4 + 0x27 <= _Var4) {
LAB_140004f82:
      FUN_140001110();
      pcVar1 = (code *)swi(3);
      (*pcVar1)();
      return;
    }
    pvVar3 = operator_new(_Var4 + 0x27);
    if (pvVar3 == (void *)0x0) {
                    /* WARNING: Subroutine does not return */
      _invoke_watson((wchar_t *)0x0,(wchar_t *)0x0,(wchar_t *)0x0,0,0);
    }
    _Dst = (void *)((longlong)pvVar3 + 0x27U & 0xffffffffffffffe0);
    *(void **)((longlong)_Dst - 8) = pvVar3;
  }
LAB_140004f43:
  param_1[2] = param_3;
  *param_1 = _Dst;
  param_1[3] = uVar2;
  memcpy(_Dst,param_2,param_3 * 2);
  *(undefined2 *)(param_3 * 2 + (longlong)_Dst) = 0;
  return;
}



// Function: FUN_140004f90 @ 140004f90
// Signature: undefined8 *FUN_140004f90(undefined8 *param_1,undefined8 param_2,undefined8 param_3,undefined1 param_4);


undefined8 *
FUN_140004f90(undefined8 *param_1,undefined8 param_2,undefined8 param_3,undefined1 param_4)

{
  ulonglong uVar1;
  size_t _Size;
  ulonglong uVar2;
  code *pcVar3;
  void *pvVar4;
  undefined8 *puVar5;
  ulonglong uVar6;
  ulonglong uVar7;
  void *_Memory;
  void *_Dst;
  
  _Size = param_1[2];
  uVar7 = 0x7fffffffffffffff;
  if (_Size == 0x7fffffffffffffff) {
    FUN_1400011b0();
    pcVar3 = (code *)swi(3);
    puVar5 = (undefined8 *)(*pcVar3)();
    return puVar5;
  }
  uVar2 = param_1[3];
  uVar6 = _Size + 1 | 0xf;
  if ((uVar6 < 0x8000000000000000) && (uVar2 <= 0x7fffffffffffffff - (uVar2 >> 1))) {
    uVar1 = uVar2 + (uVar2 >> 1);
    uVar7 = uVar6;
    if (uVar6 < uVar1) {
      uVar7 = uVar1;
    }
    uVar1 = uVar7 + 1;
    if (uVar1 == 0) {
      _Dst = (void *)0x0;
    }
    else {
      if (0xfff < uVar1) {
        uVar6 = uVar7 + 0x28;
        if (uVar6 <= uVar1) {
          FUN_140001110();
          pcVar3 = (code *)swi(3);
          puVar5 = (undefined8 *)(*pcVar3)();
          return puVar5;
        }
        goto LAB_140005037;
      }
      _Dst = operator_new(uVar1);
    }
  }
  else {
    uVar6 = 0x8000000000000027;
LAB_140005037:
    pvVar4 = operator_new(uVar6);
    if (pvVar4 == (void *)0x0) goto LAB_1400050b2;
    _Dst = (void *)((longlong)pvVar4 + 0x27U & 0xffffffffffffffe0);
    *(void **)((longlong)_Dst - 8) = pvVar4;
  }
  param_1[2] = _Size + 1;
  param_1[3] = uVar7;
  if (uVar2 < 0x10) {
    memcpy(_Dst,param_1,_Size);
    *(undefined1 *)(_Size + (longlong)_Dst) = param_4;
    *(undefined1 *)(_Size + 1 + (longlong)_Dst) = 0;
  }
  else {
    pvVar4 = (void *)*param_1;
    memcpy(_Dst,pvVar4,_Size);
    *(undefined1 *)(_Size + (longlong)_Dst) = param_4;
    *(undefined1 *)(_Size + 1 + (longlong)_Dst) = 0;
    _Memory = pvVar4;
    if ((0xfff < uVar2 + 1) &&
       (_Memory = *(void **)((longlong)pvVar4 + -8),
       0x1f < (ulonglong)((longlong)pvVar4 + (-8 - (longlong)_Memory)))) {
LAB_1400050b2:
                    /* WARNING: Subroutine does not return */
      _invoke_watson((wchar_t *)0x0,(wchar_t *)0x0,(wchar_t *)0x0,0,0);
    }
    free(_Memory);
  }
  *param_1 = _Dst;
  return param_1;
}



// Function: FUN_140005110 @ 140005110
// Signature: facet * FUN_140005110(locale *param_1);


facet * FUN_140005110(locale *param_1)

{
  ulonglong uVar1;
  longlong lVar2;
  code *pcVar3;
  code *pcVar4;
  _Locimp *p_Var5;
  __uint64 _Var6;
  facet *pfVar7;
  facet *local_res8;
  _Lockit local_res10 [8];
  facet *local_res18;
  
  std::_Lockit::_Lockit(local_res10,0);
  pcVar3 = id_exref;
  local_res18 = DAT_14000e930;
  if (*(longlong *)id_exref == 0) {
    std::_Lockit::_Lockit((_Lockit *)&local_res8,0);
    pcVar4 = _Id_cnt_exref;
    if (*(longlong *)pcVar3 == 0) {
      *(int *)_Id_cnt_exref = *(int *)_Id_cnt_exref + 1;
      *(longlong *)pcVar3 = (longlong)*(int *)pcVar4;
    }
    std::_Lockit::~_Lockit((_Lockit *)&local_res8);
  }
  uVar1 = *(ulonglong *)pcVar3;
  lVar2 = *(longlong *)(param_1 + 8);
  if (uVar1 < *(ulonglong *)(lVar2 + 0x18)) {
    pfVar7 = *(facet **)(uVar1 * 8 + *(longlong *)(lVar2 + 0x10));
    if (pfVar7 != (facet *)0x0) goto LAB_1400051fd;
  }
  else {
    pfVar7 = (facet *)0x0;
  }
  if (*(char *)(lVar2 + 0x24) == '\0') {
LAB_1400051b3:
    if (pfVar7 != (facet *)0x0) goto LAB_1400051fd;
  }
  else {
    p_Var5 = std::locale::_Getgloballocale();
    if (uVar1 < *(ulonglong *)(p_Var5 + 0x18)) {
      pfVar7 = *(facet **)(uVar1 * 8 + *(longlong *)(p_Var5 + 0x10));
      goto LAB_1400051b3;
    }
  }
  pfVar7 = local_res18;
  if (local_res18 == (facet *)0x0) {
    _Var6 = std::codecvt<char,char,struct__Mbstatet>::_Getcat(&local_res18,param_1);
    pfVar7 = local_res18;
    if (_Var6 == 0xffffffffffffffff) {
      FUN_140001200();
      pcVar3 = (code *)swi(3);
      pfVar7 = (facet *)(*pcVar3)();
      return pfVar7;
    }
    local_res8 = local_res18;
    FUN_140007554(local_res18);
    (**(code **)(*(longlong *)pfVar7 + 8))(pfVar7);
    DAT_14000e930 = local_res18;
    pfVar7 = local_res18;
  }
LAB_1400051fd:
  std::_Lockit::~_Lockit(local_res10);
  return pfVar7;
}



// Function: FUN_140005240 @ 140005240
// Signature: undefined1 (*) [16]FUN_140005240(undefined1 (*param_1) [16],undefined8 *param_2,ulonglong param_3,ulonglong param_4);


undefined1 (*) [16]
FUN_140005240(undefined1 (*param_1) [16],undefined8 *param_2,ulonglong param_3,ulonglong param_4)

{
  ulonglong uVar1;
  code *pcVar2;
  ulonglong uVar3;
  ulonglong uVar4;
  void *pvVar5;
  undefined1 (*pauVar6) [16];
  void *_Dst;
  
  _Dst = (void *)0x0;
  *param_1 = (undefined1  [16])0x0;
  *(undefined8 *)param_1[1] = 0;
  *(undefined8 *)(param_1[1] + 8) = 0;
  if ((ulonglong)param_2[2] < param_3) {
    FUN_140005f10();
    pcVar2 = (code *)swi(3);
    pauVar6 = (undefined1 (*) [16])(*pcVar2)();
    return pauVar6;
  }
  uVar3 = param_2[2] - param_3;
  if (uVar3 < param_4) {
    param_4 = uVar3;
  }
  if (0xf < (ulonglong)param_2[3]) {
    param_2 = (undefined8 *)*param_2;
  }
  if (0x7fffffffffffffff < param_4) {
    FUN_1400011b0();
    pcVar2 = (code *)swi(3);
    pauVar6 = (undefined1 (*) [16])(*pcVar2)();
    return pauVar6;
  }
  if (param_4 < 0x10) {
    *(ulonglong *)param_1[1] = param_4;
    *(undefined8 *)(param_1[1] + 8) = 0xf;
    memcpy(param_1,(void *)((longlong)param_2 + param_3),param_4);
    (*param_1)[param_4] = 0;
    return param_1;
  }
  uVar3 = param_4 | 0xf;
  if (uVar3 < 0x8000000000000000) {
    if (uVar3 < 0x16) {
      uVar3 = 0x16;
    }
    uVar1 = uVar3 + 1;
    if (uVar1 == 0) goto LAB_14000534e;
    if (uVar1 < 0x1000) {
      _Dst = operator_new(uVar1);
      goto LAB_14000534e;
    }
    uVar4 = uVar3 + 0x28;
    if (uVar4 <= uVar1) {
      FUN_140001110();
      pcVar2 = (code *)swi(3);
      pauVar6 = (undefined1 (*) [16])(*pcVar2)();
      return pauVar6;
    }
  }
  else {
    uVar4 = 0x8000000000000027;
    uVar3 = 0x7fffffffffffffff;
  }
  pvVar5 = operator_new(uVar4);
  if (pvVar5 == (void *)0x0) {
                    /* WARNING: Subroutine does not return */
    _invoke_watson((wchar_t *)0x0,(wchar_t *)0x0,(wchar_t *)0x0,0,0);
  }
  _Dst = (void *)((longlong)pvVar5 + 0x27U & 0xffffffffffffffe0);
  *(void **)((longlong)_Dst - 8) = pvVar5;
LAB_14000534e:
  *(void **)*param_1 = _Dst;
  *(ulonglong *)param_1[1] = param_4;
  *(ulonglong *)(param_1[1] + 8) = uVar3;
  memcpy(_Dst,(void *)((longlong)param_2 + param_3),param_4);
  *(undefined1 *)(param_4 + (longlong)_Dst) = 0;
  return param_1;
}



// Function: FUN_1400053a0 @ 1400053a0
// Signature: basic_istream<char,struct_std::char_traits<char>_> *FUN_1400053a0(basic_istream<char,struct_std::char_traits<char>_> *param_1,wchar_t *param_2);


basic_istream<char,struct_std::char_traits<char>_> *
FUN_1400053a0(basic_istream<char,struct_std::char_traits<char>_> *param_1,wchar_t *param_2)

{
  basic_streambuf<char,struct_std::char_traits<char>_> *this;
  bool bVar1;
  _iobuf *p_Var2;
  locale *plVar3;
  facet *this_00;
  undefined8 *puVar4;
  undefined8 local_38;
  undefined8 local_30;
  undefined8 local_28;
  longlong *local_20;
  
  *(undefined **)param_1 = &DAT_14000ad90;
  std::basic_ios<char,struct_std::char_traits<char>_>::
  basic_ios<char,struct_std::char_traits<char>_>
            ((basic_ios<char,struct_std::char_traits<char>_> *)(param_1 + 0xb0));
  this = (basic_streambuf<char,struct_std::char_traits<char>_> *)(param_1 + 0x10);
  std::basic_istream<char,struct_std::char_traits<char>_>::
  basic_istream<char,struct_std::char_traits<char>_>(param_1,this,false);
  *(undefined ***)(param_1 + *(int *)(*(longlong *)param_1 + 4)) =
       std::basic_ifstream<char,struct_std::char_traits<char>_>::vftable;
  *(int *)(param_1 + (longlong)*(int *)(*(longlong *)param_1 + 4) + -4) =
       *(int *)(*(longlong *)param_1 + 4) + -0xb0;
  std::basic_streambuf<char,struct_std::char_traits<char>_>::
  basic_streambuf<char,struct_std::char_traits<char>_>(this);
  *(undefined ***)this = std::basic_filebuf<char,struct_std::char_traits<char>_>::vftable;
  param_1[0x8c] = (basic_istream<char,struct_std::char_traits<char>_>)0x0;
  param_1[0x81] = (basic_istream<char,struct_std::char_traits<char>_>)0x0;
  std::basic_streambuf<char,struct_std::char_traits<char>_>::_Init(this);
  *(undefined8 *)(param_1 + 0x90) = 0;
  *(undefined8 *)(param_1 + 0x84) = DAT_14000e938;
  *(undefined8 *)(param_1 + 0x78) = 0;
  p_Var2 = std::_Fiopen(param_2,1,0x40);
  if (p_Var2 != (_iobuf *)0x0) {
    param_1[0x8c] = (basic_istream<char,struct_std::char_traits<char>_>)0x1;
    param_1[0x81] = (basic_istream<char,struct_std::char_traits<char>_>)0x0;
    std::basic_streambuf<char,struct_std::char_traits<char>_>::_Init(this);
    local_38 = 0;
    local_30 = 0;
    local_28 = 0;
    _get_stream_buffer_pointers(p_Var2,&local_38,&local_30,&local_28);
    *(undefined8 *)(param_1 + 0x28) = local_38;
    *(undefined8 *)(param_1 + 0x30) = local_38;
    *(undefined8 *)(param_1 + 0x48) = local_30;
    *(undefined8 *)(param_1 + 0x50) = local_30;
    *(undefined8 *)(param_1 + 0x60) = local_28;
    *(undefined8 *)(param_1 + 0x68) = local_28;
    *(_iobuf **)(param_1 + 0x90) = p_Var2;
    *(undefined8 *)(param_1 + 0x84) = DAT_14000e938;
    *(undefined8 *)(param_1 + 0x78) = 0;
    plVar3 = (locale *)std::basic_streambuf<char,struct_std::char_traits<char>_>::getloc(this);
    this_00 = FUN_140005110(plVar3);
    bVar1 = std::codecvt_base::always_noconv((codecvt_base *)this_00);
    if (bVar1) {
      *(undefined8 *)(param_1 + 0x78) = 0;
    }
    else {
      *(facet **)(param_1 + 0x78) = this_00;
      std::basic_streambuf<char,struct_std::char_traits<char>_>::_Init(this);
    }
    if ((local_20 != (longlong *)0x0) &&
       (puVar4 = (undefined8 *)(**(code **)(*local_20 + 0x10))(), puVar4 != (undefined8 *)0x0)) {
      (**(code **)*puVar4)(puVar4,1);
    }
    if (this != (basic_streambuf<char,struct_std::char_traits<char>_> *)0x0) {
      return param_1;
    }
  }
  std::basic_ios<char,struct_std::char_traits<char>_>::setstate
            ((basic_ios<char,struct_std::char_traits<char>_> *)
             (param_1 + *(int *)(*(longlong *)param_1 + 4)),2,false);
  return param_1;
}



// Function: FUN_140005580 @ 140005580
// Signature: bool FUN_140005580(longlong *param_1);


bool FUN_140005580(longlong *param_1)

{
  int iVar1;
  size_t sVar2;
  size_t _Count;
  char *local_res8;
  char local_28 [32];
  
  if ((param_1[0xd] == 0) || (*(char *)((longlong)param_1 + 0x71) == '\0')) {
    return true;
  }
  iVar1 = (**(code **)(*param_1 + 0x18))(param_1,0xffffffff);
  if (iVar1 == -1) {
    return false;
  }
  iVar1 = std::codecvt<char,char,struct__Mbstatet>::unshift
                    ((codecvt<char,char,struct__Mbstatet> *)param_1[0xd],
                     (_Mbstatet *)((longlong)param_1 + 0x74),local_28,&stack0xfffffffffffffff8,
                     &local_res8);
  if (iVar1 == 0) {
    *(undefined1 *)((longlong)param_1 + 0x71) = 0;
  }
  else if (iVar1 != 1) {
    if (iVar1 != 3) {
      return false;
    }
    *(undefined1 *)((longlong)param_1 + 0x71) = 0;
    return true;
  }
  _Count = (longlong)local_res8 - (longlong)local_28;
  if ((_Count != 0) && (sVar2 = fwrite(local_28,1,_Count,(FILE *)param_1[0x10]), _Count != sVar2)) {
    return false;
  }
  return *(char *)((longlong)param_1 + 0x71) == '\0';
}



// Function: FUN_140005660 @ 140005660
// Signature: void FUN_140005660(longlong *param_1);


void FUN_140005660(longlong *param_1)

{
  longlong *plVar1;
  
  plVar1 = *(longlong **)((longlong)*(int *)(*(longlong *)*param_1 + 4) + 0x48 + *param_1);
  if (plVar1 != (longlong *)0x0) {
    (**(code **)(*plVar1 + 0x10))();
  }
  return;
}



// Function: FUN_140005690 @ 140005690
// Signature: undefined8 * FUN_140005690(longlong *param_1,undefined8 *param_2,undefined8 *param_3);


undefined8 * FUN_140005690(longlong *param_1,undefined8 *param_2,undefined8 *param_3)

{
  ulonglong uVar1;
  undefined8 *puVar2;
  longlong lVar3;
  undefined8 *puVar4;
  code *pcVar5;
  undefined8 uVar6;
  undefined8 uVar7;
  undefined8 uVar8;
  longlong lVar9;
  void *pvVar10;
  undefined8 *puVar11;
  undefined8 *puVar12;
  ulonglong uVar13;
  __uint64 _Var14;
  undefined1 (*pauVar15) [16];
  undefined1 (*pauVar16) [16];
  void *_Memory;
  undefined1 (*pauVar17) [16];
  undefined1 (*pauVar18) [16];
  ulonglong uVar19;
  
  lVar3 = *param_1;
  lVar9 = param_1[1] - lVar3 >> 6;
  if (lVar9 == 0x3ffffffffffffff) {
    FUN_140005ef0();
    pcVar5 = (code *)swi(3);
    puVar12 = (undefined8 *)(*pcVar5)();
    return puVar12;
  }
  pauVar18 = (undefined1 (*) [16])0x0;
  uVar13 = param_1[2] - lVar3 >> 6;
  uVar1 = lVar9 + 1;
  if (0x3ffffffffffffff - (uVar13 >> 1) < uVar13) {
    uVar19 = 0xffffffffffffffc0;
    _Var14 = 0xffffffffffffffe7;
LAB_14000574a:
    pvVar10 = operator_new(_Var14);
    if (pvVar10 == (void *)0x0) goto LAB_140005a0c;
    pauVar18 = (undefined1 (*) [16])((longlong)pvVar10 + 0x27U & 0xffffffffffffffe0);
    *(void **)(pauVar18[-1] + 8) = pvVar10;
  }
  else {
    uVar13 = (uVar13 >> 1) + uVar13;
    uVar19 = uVar1;
    if (uVar1 <= uVar13) {
      uVar19 = uVar13;
    }
    if (0x3ffffffffffffff < uVar19) {
LAB_140005a28:
      FUN_140001110();
      pcVar5 = (code *)swi(3);
      puVar12 = (undefined8 *)(*pcVar5)();
      return puVar12;
    }
    uVar19 = uVar19 * 0x40;
    if (uVar19 != 0) {
      if (0xfff < uVar19) {
        _Var14 = uVar19 + 0x27;
        if (_Var14 <= uVar19) goto LAB_140005a28;
        goto LAB_14000574a;
      }
      pauVar18 = (undefined1 (*) [16])operator_new(uVar19);
    }
  }
  uVar13 = (longlong)param_2 - lVar3 & 0xffffffffffffffc0;
  *(undefined1 (*) [16])(*pauVar18 + uVar13) = (undefined1  [16])0x0;
  *(undefined8 *)(pauVar18[1] + uVar13) = 0;
  puVar12 = (undefined8 *)(*pauVar18 + uVar13);
  puVar12[3] = 0;
  pauVar15 = (undefined1 (*) [16])(puVar12 + 8);
  uVar6 = param_3[1];
  uVar7 = param_3[2];
  uVar8 = param_3[3];
  *puVar12 = *param_3;
  puVar12[1] = uVar6;
  puVar12[2] = uVar7;
  puVar12[3] = uVar8;
  param_3[3] = 7;
  *(undefined2 *)param_3 = 0;
  param_3[2] = 0;
  *(undefined1 (*) [16])(puVar12 + 4) = (undefined1  [16])0x0;
  puVar12[6] = 0;
  puVar12[7] = 0;
  uVar6 = param_3[5];
  uVar7 = param_3[6];
  uVar8 = param_3[7];
  puVar12[4] = param_3[4];
  puVar12[5] = uVar6;
  puVar12[6] = uVar7;
  puVar12[7] = uVar8;
  param_3[7] = 7;
  *(undefined2 *)(param_3 + 4) = 0;
  param_3[6] = 0;
  puVar4 = (undefined8 *)param_1[1];
  puVar11 = (undefined8 *)*param_1;
  if (param_2 == puVar4) {
    pauVar15 = pauVar18;
    if (puVar11 != puVar4) {
      pauVar16 = pauVar18 + 2;
      do {
        *pauVar15 = (undefined1  [16])0x0;
        *(undefined8 *)pauVar16[-1] = 0;
        *(undefined8 *)(pauVar16[-1] + 8) = 0;
        uVar6 = puVar11[1];
        uVar7 = puVar11[2];
        uVar8 = puVar11[3];
        *(undefined8 *)*pauVar15 = *puVar11;
        *(undefined8 *)(*pauVar15 + 8) = uVar6;
        *(undefined8 *)pauVar15[1] = uVar7;
        *(undefined8 *)(pauVar15[1] + 8) = uVar8;
        puVar11[3] = 7;
        pauVar15 = pauVar15 + 4;
        *(undefined2 *)puVar11 = 0;
        puVar11[2] = 0;
        *pauVar16 = (undefined1  [16])0x0;
        *(undefined8 *)pauVar16[1] = 0;
        *(undefined8 *)(pauVar16[1] + 8) = 0;
        uVar6 = puVar11[5];
        uVar7 = puVar11[6];
        uVar8 = puVar11[7];
        *(undefined8 *)*pauVar16 = puVar11[4];
        *(undefined8 *)(*pauVar16 + 8) = uVar6;
        *(undefined8 *)pauVar16[1] = uVar7;
        *(undefined8 *)(pauVar16[1] + 8) = uVar8;
        *(undefined2 *)(puVar11 + 4) = 0;
        puVar11[6] = 0;
        puVar11[7] = 7;
        puVar11 = puVar11 + 8;
        pauVar16 = pauVar16 + 4;
      } while (puVar11 != puVar4);
    }
    FUN_140005a30((ulonglong *)pauVar15,(ulonglong *)pauVar15);
  }
  else {
    pauVar16 = pauVar18;
    if (puVar11 != param_2) {
      pauVar17 = pauVar18 + 2;
      do {
        *pauVar16 = (undefined1  [16])0x0;
        *(undefined8 *)pauVar17[-1] = 0;
        *(undefined8 *)(pauVar17[-1] + 8) = 0;
        uVar6 = puVar11[1];
        uVar7 = puVar11[2];
        uVar8 = puVar11[3];
        *(undefined8 *)*pauVar16 = *puVar11;
        *(undefined8 *)(*pauVar16 + 8) = uVar6;
        *(undefined8 *)pauVar16[1] = uVar7;
        *(undefined8 *)(pauVar16[1] + 8) = uVar8;
        puVar11[3] = 7;
        pauVar16 = pauVar16 + 4;
        *(undefined2 *)puVar11 = 0;
        puVar11[2] = 0;
        *pauVar17 = (undefined1  [16])0x0;
        *(undefined8 *)pauVar17[1] = 0;
        *(undefined8 *)(pauVar17[1] + 8) = 0;
        uVar6 = puVar11[5];
        uVar7 = puVar11[6];
        uVar8 = puVar11[7];
        *(undefined8 *)*pauVar17 = puVar11[4];
        *(undefined8 *)(*pauVar17 + 8) = uVar6;
        *(undefined8 *)pauVar17[1] = uVar7;
        *(undefined8 *)(pauVar17[1] + 8) = uVar8;
        *(undefined2 *)(puVar11 + 4) = 0;
        puVar11[6] = 0;
        puVar11[7] = 7;
        puVar11 = puVar11 + 8;
        pauVar17 = pauVar17 + 4;
      } while (puVar11 != param_2);
    }
    FUN_140005a30((ulonglong *)pauVar16,(ulonglong *)pauVar16);
    puVar4 = (undefined8 *)param_1[1];
    if (param_2 != puVar4) {
      puVar11 = param_2 + 3;
      pauVar16 = (undefined1 (*) [16])(puVar12 + 0xc);
      do {
        *pauVar15 = (undefined1  [16])0x0;
        *(undefined8 *)pauVar16[-1] = 0;
        *(undefined8 *)(pauVar16[-1] + 8) = 0;
        puVar2 = puVar11 + 5;
        uVar6 = puVar11[-2];
        uVar7 = puVar11[-1];
        uVar8 = *puVar11;
        *(undefined8 *)*pauVar15 = puVar11[-3];
        *(undefined8 *)(*pauVar15 + 8) = uVar6;
        *(undefined8 *)pauVar15[1] = uVar7;
        *(undefined8 *)(pauVar15[1] + 8) = uVar8;
        *puVar11 = 7;
        *(undefined2 *)(puVar11 + -3) = 0;
        pauVar15 = pauVar15 + 4;
        puVar11[-1] = 0;
        *pauVar16 = (undefined1  [16])0x0;
        *(undefined8 *)pauVar16[1] = 0;
        *(undefined8 *)(pauVar16[1] + 8) = 0;
        uVar6 = puVar11[2];
        uVar7 = puVar11[3];
        uVar8 = puVar11[4];
        *(undefined8 *)*pauVar16 = puVar11[1];
        *(undefined8 *)(*pauVar16 + 8) = uVar6;
        *(undefined8 *)pauVar16[1] = uVar7;
        *(undefined8 *)(pauVar16[1] + 8) = uVar8;
        *(undefined2 *)(puVar11 + 1) = 0;
        puVar11[3] = 0;
        puVar11[4] = 7;
        puVar11 = puVar11 + 8;
        pauVar16 = pauVar16 + 4;
      } while (puVar2 != puVar4);
    }
    FUN_140005a30((ulonglong *)pauVar15,(ulonglong *)pauVar15);
  }
  if ((ulonglong *)*param_1 != (ulonglong *)0x0) {
    FUN_140005a30((ulonglong *)*param_1,(ulonglong *)param_1[1]);
    pvVar10 = (void *)*param_1;
    _Memory = pvVar10;
    if ((0xfff < (param_1[2] - (longlong)pvVar10 & 0xffffffffffffffc0U)) &&
       (_Memory = *(void **)((longlong)pvVar10 + -8),
       0x1f < (ulonglong)((longlong)pvVar10 + (-8 - (longlong)_Memory)))) {
LAB_140005a0c:
                    /* WARNING: Subroutine does not return */
      _invoke_watson((wchar_t *)0x0,(wchar_t *)0x0,(wchar_t *)0x0,0,0);
    }
    free(_Memory);
  }
  *param_1 = (longlong)pauVar18;
  param_1[1] = (longlong)(pauVar18 + uVar1 * 4);
  param_1[2] = (longlong)(*pauVar18 + uVar19);
  return puVar12;
}



// Function: FUN_140005a30 @ 140005a30
// Signature: void FUN_140005a30(ulonglong *param_1,ulonglong *param_2);


void FUN_140005a30(ulonglong *param_1,ulonglong *param_2)

{
  ulonglong *puVar1;
  void *pvVar2;
  void *pvVar3;
  ulonglong *puVar4;
  
  if (param_1 != param_2) {
    puVar4 = param_1 + 7;
    do {
      if (7 < *puVar4) {
        pvVar2 = (void *)puVar4[-3];
        pvVar3 = pvVar2;
        if ((0xfff < *puVar4 * 2 + 2) &&
           (pvVar3 = *(void **)((longlong)pvVar2 - 8),
           0x1f < (ulonglong)((longlong)pvVar2 + (-8 - (longlong)pvVar3)))) goto LAB_140005b16;
        free(pvVar3);
      }
      puVar4[-1] = 0;
      *puVar4 = 7;
      *(undefined2 *)(puVar4 + -3) = 0;
      if (7 < puVar4[-4]) {
        pvVar2 = (void *)puVar4[-7];
        pvVar3 = pvVar2;
        if ((0xfff < puVar4[-4] * 2 + 2) &&
           (pvVar3 = *(void **)((longlong)pvVar2 - 8),
           0x1f < (ulonglong)((longlong)pvVar2 + (-8 - (longlong)pvVar3)))) {
LAB_140005b16:
                    /* WARNING: Subroutine does not return */
          _invoke_watson((wchar_t *)0x0,(wchar_t *)0x0,(wchar_t *)0x0,0,0);
        }
        free(pvVar3);
      }
      puVar4[-5] = 0;
      puVar4[-4] = 7;
      *(undefined2 *)(puVar4 + -7) = 0;
      puVar1 = puVar4 + 1;
      puVar4 = puVar4 + 8;
    } while (puVar1 != param_2);
  }
  return;
}



// Function: FUN_140005b30 @ 140005b30
// Signature: void FUN_140005b30(longlong *param_1);


void FUN_140005b30(longlong *param_1)

{
  longlong *plVar1;
  
  plVar1 = *(longlong **)((longlong)*(int *)(*(longlong *)*param_1 + 4) + 0x48 + *param_1);
  if (plVar1 != (longlong *)0x0) {
    (**(code **)(*plVar1 + 0x10))();
  }
  return;
}



// Function: FUN_140005b60 @ 140005b60
// Signature: undefined8 * FUN_140005b60(undefined8 *param_1,ulonglong param_2);


undefined8 * FUN_140005b60(undefined8 *param_1,ulonglong param_2)

{
  ulonglong uVar1;
  size_t _Size;
  longlong lVar2;
  ulonglong uVar3;
  code *pcVar4;
  size_t _Size_00;
  void *pvVar5;
  undefined8 *puVar6;
  __uint64 _Var7;
  ulonglong uVar8;
  ulonglong uVar9;
  void *_Memory;
  void *_Dst;
  longlong in_stack_00000030;
  
  lVar2 = param_1[2];
  uVar9 = 0x7ffffffffffffffe;
  if (0x7ffffffffffffffeU - lVar2 < param_2) {
    FUN_1400011b0();
    pcVar4 = (code *)swi(3);
    puVar6 = (undefined8 *)(*pcVar4)();
    return puVar6;
  }
  uVar8 = lVar2 + param_2 | 7;
  uVar3 = param_1[3];
  if ((uVar8 < 0x7fffffffffffffff) && (uVar3 <= 0x7ffffffffffffffe - (uVar3 >> 1))) {
    uVar1 = uVar3 + (uVar3 >> 1);
    uVar9 = uVar8;
    if (uVar8 < uVar1) {
      uVar9 = uVar1;
    }
    if (0x7fffffffffffffff < uVar9 + 1) goto LAB_140005d08;
    _Var7 = (uVar9 + 1) * 2;
    if (_Var7 != 0) goto LAB_140005c03;
    _Dst = (void *)0x0;
  }
  else {
    _Var7 = 0xfffffffffffffffe;
LAB_140005c03:
    if (_Var7 < 0x1000) {
      _Dst = operator_new(_Var7);
    }
    else {
      if (_Var7 + 0x27 <= _Var7) {
LAB_140005d08:
        FUN_140001110();
        pcVar4 = (code *)swi(3);
        puVar6 = (undefined8 *)(*pcVar4)();
        return puVar6;
      }
      pvVar5 = operator_new(_Var7 + 0x27);
      if (pvVar5 == (void *)0x0) goto LAB_140005cb1;
      _Dst = (void *)((longlong)pvVar5 + 0x27U & 0xffffffffffffffe0);
      *(void **)((longlong)_Dst - 8) = pvVar5;
    }
  }
  param_1[2] = lVar2 + param_2;
  _Size_00 = in_stack_00000030 * 2;
  param_1[3] = uVar9;
  _Size = lVar2 * 2 + 2;
  if (uVar3 < 8) {
    memcpy(_Dst,L"Failed to launch Helios2.exe. Error: ",_Size_00);
    memcpy((void *)(_Size_00 + (longlong)_Dst),param_1,_Size);
  }
  else {
    pvVar5 = (void *)*param_1;
    memcpy(_Dst,L"Failed to launch Helios2.exe. Error: ",_Size_00);
    memcpy((void *)(_Size_00 + (longlong)_Dst),pvVar5,_Size);
    _Memory = pvVar5;
    if ((0xfff < uVar3 * 2 + 2) &&
       (_Memory = *(void **)((longlong)pvVar5 + -8),
       0x1f < (ulonglong)((longlong)pvVar5 + (-8 - (longlong)_Memory)))) {
LAB_140005cb1:
                    /* WARNING: Subroutine does not return */
      _invoke_watson((wchar_t *)0x0,(wchar_t *)0x0,(wchar_t *)0x0,0,0);
    }
    free(_Memory);
  }
  *param_1 = _Dst;
  return param_1;
}



// Function: FUN_140005d10 @ 140005d10
// Signature: undefined8 *FUN_140005d10(undefined8 *param_1,ulonglong param_2,undefined8 param_3,void *param_4,longlong param_5);


undefined8 *
FUN_140005d10(undefined8 *param_1,ulonglong param_2,undefined8 param_3,void *param_4,
             longlong param_5)

{
  ulonglong uVar1;
  undefined2 *puVar2;
  longlong lVar3;
  ulonglong uVar4;
  code *pcVar5;
  size_t _Size;
  void *pvVar6;
  undefined8 *puVar7;
  __uint64 _Var8;
  ulonglong uVar9;
  ulonglong uVar10;
  void *_Memory;
  void *_Dst;
  
  lVar3 = param_1[2];
  uVar10 = 0x7ffffffffffffffe;
  if (0x7ffffffffffffffeU - lVar3 < param_2) {
    FUN_1400011b0();
    pcVar5 = (code *)swi(3);
    puVar7 = (undefined8 *)(*pcVar5)();
    return puVar7;
  }
  _Dst = (void *)0x0;
  uVar4 = param_1[3];
  uVar9 = lVar3 + param_2 | 7;
  if ((uVar9 < 0x7fffffffffffffff) && (uVar4 <= 0x7ffffffffffffffe - (uVar4 >> 1))) {
    uVar1 = uVar4 + (uVar4 >> 1);
    uVar10 = uVar9;
    if (uVar9 < uVar1) {
      uVar10 = uVar1;
    }
    if (0x7fffffffffffffff < uVar10 + 1) goto LAB_140005edd;
    _Var8 = (uVar10 + 1) * 2;
    if (_Var8 != 0) goto LAB_140005dc0;
  }
  else {
    _Var8 = 0xfffffffffffffffe;
LAB_140005dc0:
    if (_Var8 < 0x1000) {
      _Dst = operator_new(_Var8);
    }
    else {
      if (_Var8 + 0x27 <= _Var8) {
LAB_140005edd:
        FUN_140001110();
        pcVar5 = (code *)swi(3);
        puVar7 = (undefined8 *)(*pcVar5)();
        return puVar7;
      }
      pvVar6 = operator_new(_Var8 + 0x27);
      if (pvVar6 == (void *)0x0) goto LAB_140005e7d;
      _Dst = (void *)((longlong)pvVar6 + 0x27U & 0xffffffffffffffe0);
      *(void **)((longlong)_Dst - 8) = pvVar6;
    }
  }
  param_1[2] = lVar3 + param_2;
  _Size = lVar3 * 2;
  param_1[3] = uVar10;
  puVar2 = (undefined2 *)((longlong)_Dst + (lVar3 + param_5) * 2);
  if (uVar4 < 8) {
    memcpy(_Dst,param_1,_Size);
    memcpy((void *)(_Size + (longlong)_Dst),param_4,param_5 * 2);
    *puVar2 = 0;
  }
  else {
    pvVar6 = (void *)*param_1;
    memcpy(_Dst,pvVar6,_Size);
    memcpy((void *)(_Size + (longlong)_Dst),param_4,param_5 * 2);
    *puVar2 = 0;
    _Memory = pvVar6;
    if ((0xfff < uVar4 * 2 + 2) &&
       (_Memory = *(void **)((longlong)pvVar6 + -8),
       0x1f < (ulonglong)((longlong)pvVar6 + (-8 - (longlong)_Memory)))) {
LAB_140005e7d:
                    /* WARNING: Subroutine does not return */
      _invoke_watson((wchar_t *)0x0,(wchar_t *)0x0,(wchar_t *)0x0,0,0);
    }
    free(_Memory);
  }
  *param_1 = _Dst;
  return param_1;
}



// Function: FUN_140005ef0 @ 140005ef0
// Signature: void FUN_140005ef0(void);


void FUN_140005ef0(void)

{
  code *pcVar1;
  
  std::_Xlength_error("vector too long");
  pcVar1 = (code *)swi(3);
  (*pcVar1)();
  return;
}



// Function: FUN_140005f10 @ 140005f10
// Signature: void FUN_140005f10(void);


void FUN_140005f10(void)

{
  code *pcVar1;
  
  std::_Xout_of_range("invalid string position");
  pcVar1 = (code *)swi(3);
  (*pcVar1)();
  return;
}



// Function: FUN_140005f24 @ 140005f24
// Signature: void FUN_140005f24(longlong param_1,uint param_2);


void FUN_140005f24(longlong param_1,uint param_2)

{
  FUN_140004a20(param_1 - *(int *)(param_1 + -4),param_2);
  return;
}



// Function: operator_new @ 140005f30
// Signature: void * __cdecl operator_new(__uint64 param_1);


/* Library Function - Single Match
    void * __ptr64 __cdecl operator new(unsigned __int64)
   
   Libraries: Visual Studio 2017 Release, Visual Studio 2019 Release */

void * __cdecl operator_new(__uint64 param_1)

{
  code *pcVar1;
  int iVar2;
  void *pvVar3;
  
  do {
    pvVar3 = malloc(param_1);
    if (pvVar3 != (void *)0x0) {
      return pvVar3;
    }
    iVar2 = _callnewh(param_1);
  } while (iVar2 != 0);
  if (param_1 == 0xffffffffffffffff) {
    FUN_140001110();
    pcVar1 = (code *)swi(3);
    pvVar3 = (void *)(*pcVar1)();
    return pvVar3;
  }
  FUN_140006238();
  pcVar1 = (code *)swi(3);
  pvVar3 = (void *)(*pcVar1)();
  return pvVar3;
}



// Function: free @ 140005f6c
// Signature: void __cdecl free(void *_Memory);


void __cdecl free(void *_Memory)

{
                    /* WARNING: Could not recover jumptable at 0x0001400075e3. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  free(_Memory);
  return;
}



// Function: free @ 140005f74
// Signature: void __cdecl free(void *_Memory);


void __cdecl free(void *_Memory)

{
                    /* WARNING: Could not recover jumptable at 0x0001400075e3. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  free(_Memory);
  return;
}



// Function: FUN_140005f7c @ 140005f7c
// Signature: undefined8 * FUN_140005f7c(undefined8 *param_1,ulonglong param_2);


undefined8 * FUN_140005f7c(undefined8 *param_1,ulonglong param_2)

{
  *param_1 = type_info::vftable;
  if ((param_2 & 1) != 0) {
    free(param_1);
  }
  return param_1;
}



// Function: FUN_140005fa8 @ 140005fa8
// Signature: void FUN_140005fa8(void);


void FUN_140005fa8(void)

{
  code *pcVar1;
  bool bVar2;
  char cVar3;
  int iVar4;
  undefined8 uVar5;
  undefined4 *puVar6;
  ulonglong uVar7;
  undefined7 extraout_var;
  
  _set_app_type(2);
  uVar5 = FUN_140006554();
  _set_fmode((int)uVar5);
  uVar5 = FUN_140006548();
  puVar6 = (undefined4 *)__p__commode();
  *puVar6 = (int)uVar5;
  uVar5 = __scrt_initialize_onexit_tables(1);
  if ((char)uVar5 != '\0') {
    FUN_140006820();
    atexit((_func_5014 *)&LAB_14000685c);
    uVar7 = FUN_14000654c();
    iVar4 = _configure_narrow_argv(uVar7 & 0xffffffff);
    if (iVar4 == 0) {
      FUN_14000655c();
      bVar2 = FUN_1400065a0();
      if ((int)CONCAT71(extraout_var,bVar2) != 0) {
        __setusermatherr(FUN_140006548);
      }
      _guard_check_icall();
      _guard_check_icall();
      uVar5 = FUN_140006548();
      _configthreadlocale((int)uVar5);
      cVar3 = FUN_14000656c();
      if (cVar3 != '\0') {
        _initialize_narrow_environment();
      }
      FUN_140006548();
      uVar5 = thunk_FUN_140006548();
      if ((int)uVar5 == 0) {
        return;
      }
    }
  }
  FUN_1400065c8(7);
  pcVar1 = (code *)swi(3);
  (*pcVar1)();
  return;
}



// Function: FUN_140006060 @ 140006060
// Signature: undefined8 FUN_140006060(void);


undefined8 FUN_140006060(void)

{
  FUN_140006584();
  return 0;
}



// Function: FUN_14000608c @ 14000608c
// Signature: ulonglong FUN_14000608c(void);


ulonglong FUN_14000608c(void)

{
  bool bVar1;
  bool bVar2;
  int iVar3;
  undefined8 uVar4;
  longlong *plVar5;
  ulonglong uVar6;
  IMAGE_DOS_HEADER *pIVar7;
  undefined8 unaff_RBX;
  
  iVar3 = (int)unaff_RBX;
  uVar4 = FUN_140006294(1);
  if ((char)uVar4 == '\0') {
    FUN_1400065c8(7);
  }
  else {
    bVar1 = false;
    uVar4 = __scrt_acquire_startup_lock();
    iVar3 = (int)CONCAT71((int7)((ulonglong)unaff_RBX >> 8),(char)uVar4);
    if (DAT_14000e330 != 1) {
      if (DAT_14000e330 == 0) {
        DAT_14000e330 = 1;
        iVar3 = _initterm_e(&DAT_140008468,&DAT_140008480);
        if (iVar3 != 0) {
          return 0xff;
        }
        _initterm(&DAT_140008448);
        DAT_14000e330 = 2;
      }
      else {
        bVar1 = true;
      }
      __scrt_release_startup_lock((char)uVar4);
      plVar5 = (longlong *)FUN_1400065ac();
      if ((*plVar5 != 0) && (uVar6 = FUN_14000635c((longlong)plVar5), (char)uVar6 != '\0')) {
        (*(code *)PTR__guard_dispatch_icall_140008420)(0);
      }
      plVar5 = (longlong *)FUN_1400065b4();
      if ((*plVar5 != 0) && (uVar6 = FUN_14000635c((longlong)plVar5), (char)uVar6 != '\0')) {
        _register_thread_local_exe_atexit_callback(*plVar5);
      }
      __scrt_get_show_window_mode();
      _get_narrow_winmain_command_line();
      pIVar7 = &IMAGE_DOS_HEADER_140000000;
      uVar6 = FUN_140002cc0();
      iVar3 = (int)uVar6;
      bVar2 = FUN_140006758();
      if (bVar2) {
        if (!bVar1) {
          _cexit();
        }
        __scrt_uninitialize_crt(CONCAT71((int7)((ulonglong)pIVar7 >> 8),1),'\0');
        return uVar6 & 0xffffffff;
      }
      goto LAB_1400061ed;
    }
  }
  FUN_1400065c8(7);
LAB_1400061ed:
                    /* WARNING: Subroutine does not return */
  exit(iVar3);
}



// Function: entry @ 140006200
// Signature: void entry(void);


void entry(void)

{
  FUN_140006498();
  FUN_14000608c();
  return;
}



// Function: FUN_140006214 @ 140006214
// Signature: undefined8 * FUN_140006214(undefined8 *param_1);


undefined8 * FUN_140006214(undefined8 *param_1)

{
  param_1[2] = 0;
  param_1[1] = "bad allocation";
  *param_1 = std::bad_alloc::vftable;
  return param_1;
}



// Function: FUN_140006238 @ 140006238
// Signature: void FUN_140006238(void);


void FUN_140006238(void)

{
  undefined8 local_28 [5];
  
  FUN_140006214(local_28);
                    /* WARNING: Subroutine does not return */
  _CxxThrowException(local_28,(ThrowInfo *)&DAT_14000c1d0);
}



// Function: __scrt_acquire_startup_lock @ 140006258
// Signature: ulonglong __scrt_acquire_startup_lock(void);


/* Library Function - Single Match
    __scrt_acquire_startup_lock
   
   Libraries: Visual Studio 2015 Release, Visual Studio 2017 Release, Visual Studio 2019 Release */

ulonglong __scrt_acquire_startup_lock(void)

{
  ulonglong uVar1;
  bool bVar2;
  undefined7 extraout_var;
  ulonglong uVar3;
  
  bVar2 = __scrt_is_ucrt_dll_in_use();
  uVar3 = CONCAT71(extraout_var,bVar2);
  if ((int)uVar3 == 0) {
LAB_140006286:
    uVar3 = uVar3 & 0xffffffffffffff00;
  }
  else {
    do {
      uVar3 = 0;
      LOCK();
      bVar2 = DAT_14000e338 == 0;
      uVar1 = *(ulonglong *)((longlong)Self + 8);
      if (!bVar2) {
        uVar3 = DAT_14000e338;
        uVar1 = DAT_14000e338;
      }
      DAT_14000e338 = uVar1;
      UNLOCK();
      if (bVar2) goto LAB_140006286;
    } while (*(ulonglong *)((longlong)Self + 8) != uVar3);
    uVar3 = CONCAT71((int7)(uVar3 >> 8),1);
  }
  return uVar3;
}



// Function: FUN_140006294 @ 140006294
// Signature: longlong FUN_140006294(int param_1);


longlong FUN_140006294(int param_1)

{
  char cVar1;
  uint7 extraout_var;
  uint7 uVar2;
  undefined7 extraout_var_00;
  uint7 extraout_var_01;
  
  if (param_1 == 0) {
    DAT_14000e340 = 1;
  }
  FUN_140006898();
  cVar1 = FUN_14000656c();
  uVar2 = extraout_var;
  if (cVar1 != '\0') {
    cVar1 = FUN_14000656c();
    if (cVar1 != '\0') {
      return CONCAT71(extraout_var_00,1);
    }
    FUN_14000656c();
    uVar2 = extraout_var_01;
  }
  return (ulonglong)uVar2 << 8;
}



// Function: __scrt_initialize_onexit_tables @ 1400062d0
// Signature: undefined8 __scrt_initialize_onexit_tables(uint param_1);


/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */
/* Library Function - Single Match
    __scrt_initialize_onexit_tables
   
   Library: Visual Studio 2019 Release */

undefined8 __scrt_initialize_onexit_tables(uint param_1)

{
  code *pcVar1;
  bool bVar2;
  ulonglong in_RAX;
  undefined7 extraout_var;
  undefined8 uVar3;
  
  if (DAT_14000e341 == '\0') {
    if (1 < param_1) {
      FUN_1400065c8(5);
      pcVar1 = (code *)swi(3);
      uVar3 = (*pcVar1)();
      return uVar3;
    }
    bVar2 = __scrt_is_ucrt_dll_in_use();
    if (((int)CONCAT71(extraout_var,bVar2) == 0) || (param_1 != 0)) {
      in_RAX = 0xffffffffffffffff;
      DAT_14000e348 = _DAT_140008530;
      uRam000000014000e350 = _UNK_140008538;
      _DAT_14000e358 = 0xffffffffffffffff;
      _DAT_14000e360 = _DAT_140008530;
      uRam000000014000e368 = _UNK_140008538;
      _DAT_14000e370 = 0xffffffffffffffff;
    }
    else {
      in_RAX = _initialize_onexit_table(&DAT_14000e348);
      if (((int)in_RAX != 0) ||
         (in_RAX = _initialize_onexit_table(&DAT_14000e360), (int)in_RAX != 0)) {
        return in_RAX & 0xffffffffffffff00;
      }
    }
    DAT_14000e341 = '\x01';
  }
  return CONCAT71((int7)(in_RAX >> 8),1);
}



// Function: FUN_14000635c @ 14000635c
// Signature: ulonglong FUN_14000635c(longlong param_1);


ulonglong FUN_14000635c(longlong param_1)

{
  word *pwVar1;
  ulonglong uVar2;
  uint7 uVar3;
  longlong lVar4;
  word *pwVar5;
  
  uVar2 = 0x5a4d;
  if (IMAGE_DOS_HEADER_140000000.e_magic == (char  [2])0x5a4d) {
    lVar4 = (longlong)(int)IMAGE_DOS_HEADER_140000000.e_lfanew;
    if ((*(int *)(IMAGE_DOS_HEADER_140000000.e_magic + lVar4) == 0x4550) &&
       (uVar2 = 0x20b,
       *(short *)((longlong)IMAGE_DOS_HEADER_140000000.e_res_4_ + lVar4 + -4) == 0x20b)) {
      pwVar5 = (word *)(IMAGE_DOS_HEADER_140000000.e_magic + lVar4 +
                       (ulonglong)
                       *(ushort *)((longlong)IMAGE_DOS_HEADER_140000000.e_res_4_ + lVar4 + -8) +
                       0x18);
      uVar2 = (ulonglong)*(ushort *)(IMAGE_DOS_HEADER_140000000.e_magic + lVar4 + 6);
      pwVar1 = pwVar5 + uVar2 * 0x14;
      for (; pwVar5 != pwVar1; pwVar5 = pwVar5 + 0x14) {
        if (((ulonglong)*(uint *)(pwVar5 + 6) <= param_1 - 0x140000000U) &&
           (uVar2 = (ulonglong)(*(int *)(pwVar5 + 4) + *(uint *)(pwVar5 + 6)),
           param_1 - 0x140000000U < uVar2)) goto LAB_1400063d2;
      }
      pwVar5 = (word *)0x0;
LAB_1400063d2:
      if (pwVar5 == (word *)0x0) {
        return uVar2 & 0xffffffffffffff00;
      }
      uVar3 = (uint7)(uVar2 >> 8);
      if (*(int *)(pwVar5 + 0x12) < 0) {
        return (ulonglong)uVar3 << 8;
      }
      return CONCAT71(uVar3,1);
    }
  }
  return uVar2 & 0xffffffffffffff00;
}



// Function: __scrt_release_startup_lock @ 1400063f4
// Signature: void __scrt_release_startup_lock(char param_1);


/* Library Function - Single Match
    __scrt_release_startup_lock
   
   Libraries: Visual Studio 2015 Release, Visual Studio 2017 Release, Visual Studio 2019 Release */

void __scrt_release_startup_lock(char param_1)

{
  bool bVar1;
  undefined3 extraout_var;
  
  bVar1 = __scrt_is_ucrt_dll_in_use();
  if ((CONCAT31(extraout_var,bVar1) != 0) && (param_1 == '\0')) {
    LOCK();
    DAT_14000e338 = 0;
    UNLOCK();
  }
  return;
}



// Function: __scrt_uninitialize_crt @ 140006418
// Signature: undefined1 __scrt_uninitialize_crt(undefined8 param_1,char param_2);


/* Library Function - Single Match
    __scrt_uninitialize_crt
   
   Library: Visual Studio 2019 Release */

undefined1 __scrt_uninitialize_crt(undefined8 param_1,char param_2)

{
  if ((DAT_14000e340 == '\0') || (param_2 == '\0')) {
    FUN_14000656c();
    FUN_14000656c();
  }
  return 1;
}



// Function: _onexit @ 140006444
// Signature: _onexit_t __cdecl _onexit(_onexit_t _Func);


/* Library Function - Single Match
    _onexit
   
   Library: Visual Studio 2019 Release */

_onexit_t __cdecl _onexit(_onexit_t _Func)

{
  int iVar1;
  _onexit_t p_Var2;
  
  if (DAT_14000e348 == -1) {
    iVar1 = _crt_atexit();
  }
  else {
    iVar1 = _register_onexit_function(&DAT_14000e348);
  }
  p_Var2 = (_onexit_t)0x0;
  if (iVar1 == 0) {
    p_Var2 = _Func;
  }
  return p_Var2;
}



// Function: atexit @ 140006480
// Signature: int __cdecl atexit(_func_5014 *param_1);


/* Library Function - Single Match
    atexit
   
   Library: Visual Studio 2019 Release */

int __cdecl atexit(_func_5014 *param_1)

{
  _onexit_t p_Var1;
  
  p_Var1 = _onexit((_onexit_t)param_1);
  return (p_Var1 != (_onexit_t)0x0) - 1;
}



// Function: FUN_140006498 @ 140006498
// Signature: void FUN_140006498(void);


void FUN_140006498(void)

{
  DWORD DVar1;
  _FILETIME local_res8;
  LARGE_INTEGER local_res10;
  _FILETIME local_18 [2];
  
  if (DAT_14000e040 == 0x2b992ddfa232) {
    local_res8.dwLowDateTime = 0;
    local_res8.dwHighDateTime = 0;
    GetSystemTimeAsFileTime(&local_res8);
    local_18[0] = local_res8;
    DVar1 = GetCurrentThreadId();
    local_18[0] = (_FILETIME)((ulonglong)local_18[0] ^ (ulonglong)DVar1);
    DVar1 = GetCurrentProcessId();
    local_18[0] = (_FILETIME)((ulonglong)local_18[0] ^ (ulonglong)DVar1);
    QueryPerformanceCounter(&local_res10);
    DAT_14000e040 =
         ((ulonglong)local_res10.s.LowPart << 0x20 ^
          CONCAT44(local_res10.s.HighPart,local_res10.s.LowPart) ^ (ulonglong)local_18[0] ^
         (ulonglong)local_18) & 0xffffffffffff;
    if (DAT_14000e040 == 0x2b992ddfa232) {
      DAT_14000e040 = 0x2b992ddfa233;
    }
  }
  DAT_14000e080 = ~DAT_14000e040;
  return;
}



// Function: FUN_140006548 @ 140006548
// Signature: undefined8 FUN_140006548(void);


undefined8 FUN_140006548(void)

{
  return 0;
}



// Function: FUN_14000654c @ 14000654c
// Signature: undefined8 FUN_14000654c(void);


undefined8 FUN_14000654c(void)

{
  return 1;
}



// Function: FUN_140006554 @ 140006554
// Signature: undefined8 FUN_140006554(void);


undefined8 FUN_140006554(void)

{
  return 0x4000;
}



// Function: FUN_14000655c @ 14000655c
// Signature: void FUN_14000655c(void);


void FUN_14000655c(void)

{
                    /* WARNING: Could not recover jumptable at 0x000140006563. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  InitializeSListHead(&DAT_14000e380);
  return;
}



// Function: FUN_14000656c @ 14000656c
// Signature: undefined1 FUN_14000656c(void);


undefined1 FUN_14000656c(void)

{
  return 1;
}



// Function: _guard_check_icall @ 140006570
// Signature: void _guard_check_icall(void);


void _guard_check_icall(void)

{
  return;
}



// Function: FUN_140006574 @ 140006574
// Signature: undefined * FUN_140006574(void);


undefined * FUN_140006574(void)

{
  return &DAT_14000e390;
}



// Function: FUN_14000657c @ 14000657c
// Signature: undefined * FUN_14000657c(void);


undefined * FUN_14000657c(void)

{
  return &DAT_14000e398;
}



// Function: FUN_140006584 @ 140006584
// Signature: void FUN_140006584(void);


void FUN_140006584(void)

{
  ulonglong *puVar1;
  
  puVar1 = (ulonglong *)FUN_140006574();
  *puVar1 = *puVar1 | 0x24;
  puVar1 = (ulonglong *)FUN_14000657c();
  *puVar1 = *puVar1 | 2;
  return;
}



// Function: FUN_1400065a0 @ 1400065a0
// Signature: bool FUN_1400065a0(void);


bool FUN_1400065a0(void)

{
  return DAT_14000e004 == 0;
}



// Function: FUN_1400065ac @ 1400065ac
// Signature: undefined * FUN_1400065ac(void);


undefined * FUN_1400065ac(void)

{
  return &DAT_14000e948;
}



// Function: FUN_1400065b4 @ 1400065b4
// Signature: undefined * FUN_1400065b4(void);


undefined * FUN_1400065b4(void)

{
  return &DAT_14000e940;
}



// Function: FUN_1400065bc @ 1400065bc
// Signature: void FUN_1400065bc(void);


/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_1400065bc(void)

{
  _DAT_14000e3a0 = 0;
  return;
}



// Function: FUN_1400065c8 @ 1400065c8
// Signature: void FUN_1400065c8(undefined4 param_1);


void FUN_1400065c8(undefined4 param_1)

{
  code *pcVar1;
  BOOL BVar2;
  LONG LVar3;
  PRUNTIME_FUNCTION FunctionEntry;
  undefined1 *puVar4;
  undefined8 unaff_retaddr;
  DWORD64 local_res10;
  undefined1 local_res18 [8];
  undefined1 local_res20 [8];
  undefined1 auStack_5c8 [8];
  undefined1 auStack_5c0 [232];
  undefined1 local_4d8 [152];
  undefined1 *local_440;
  DWORD64 local_3e0;
  
  puVar4 = auStack_5c8;
  BVar2 = IsProcessorFeaturePresent(0x17);
  if (BVar2 != 0) {
    pcVar1 = (code *)swi(0x29);
    (*pcVar1)(param_1);
    puVar4 = auStack_5c0;
  }
  *(undefined8 *)(puVar4 + -8) = 0x1400065fc;
  FUN_1400065bc();
  *(undefined8 *)(puVar4 + -8) = 0x14000660d;
  memset(local_4d8,0,0x4d0);
  *(undefined8 *)(puVar4 + -8) = 0x140006617;
  RtlCaptureContext(local_4d8);
  *(undefined8 *)(puVar4 + -8) = 0x140006631;
  FunctionEntry = RtlLookupFunctionEntry(local_3e0,&local_res10,(PUNWIND_HISTORY_TABLE)0x0);
  if (FunctionEntry != (PRUNTIME_FUNCTION)0x0) {
    *(undefined8 *)(puVar4 + 0x38) = 0;
    *(undefined1 **)(puVar4 + 0x30) = local_res18;
    *(undefined1 **)(puVar4 + 0x28) = local_res20;
    *(undefined1 **)(puVar4 + 0x20) = local_4d8;
    *(undefined8 *)(puVar4 + -8) = 0x140006675;
    RtlVirtualUnwind(0,local_res10,local_3e0,FunctionEntry,*(PCONTEXT *)(puVar4 + 0x20),
                     *(PVOID **)(puVar4 + 0x28),*(PDWORD64 *)(puVar4 + 0x30),
                     *(PKNONVOLATILE_CONTEXT_POINTERS *)(puVar4 + 0x38));
  }
  local_440 = &stack0x00000008;
  *(undefined8 *)(puVar4 + -8) = 0x1400066a7;
  memset(puVar4 + 0x50,0,0x98);
  *(undefined8 *)(puVar4 + 0x60) = unaff_retaddr;
  *(undefined4 *)(puVar4 + 0x50) = 0x40000015;
  *(undefined4 *)(puVar4 + 0x54) = 1;
  *(undefined8 *)(puVar4 + -8) = 0x1400066c9;
  BVar2 = IsDebuggerPresent();
  *(undefined1 **)(puVar4 + 0x40) = puVar4 + 0x50;
  *(undefined1 **)(puVar4 + 0x48) = local_4d8;
  *(undefined8 *)(puVar4 + -8) = 0x1400066e6;
  SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)0x0);
  *(undefined8 *)(puVar4 + -8) = 0x1400066f1;
  LVar3 = UnhandledExceptionFilter((_EXCEPTION_POINTERS *)(puVar4 + 0x40));
  if ((LVar3 == 0) && (BVar2 != 1)) {
    *(undefined8 *)(puVar4 + -8) = 0x140006702;
    FUN_1400065bc();
  }
  return;
}



// Function: __scrt_get_show_window_mode @ 140006714
// Signature: WORD __scrt_get_show_window_mode(void);


/* Library Function - Single Match
    __scrt_get_show_window_mode
   
   Libraries: Visual Studio 2017 Release, Visual Studio 2019 Release */

WORD __scrt_get_show_window_mode(void)

{
  WORD WVar1;
  _STARTUPINFOW local_78;
  
  memset(&local_78,0,0x68);
  GetStartupInfoW(&local_78);
  WVar1 = 10;
  if (((byte)local_78.dwFlags & 1) != 0) {
    WVar1 = local_78.wShowWindow;
  }
  return WVar1;
}



// Function: thunk_FUN_140006548 @ 140006750
// Signature: undefined8 thunk_FUN_140006548(void);


undefined8 thunk_FUN_140006548(void)

{
  return 0;
}



// Function: FUN_140006758 @ 140006758
// Signature: bool FUN_140006758(void);


bool FUN_140006758(void)

{
  HMODULE pHVar1;
  longlong lVar2;
  bool bVar3;
  
  pHVar1 = GetModuleHandleW((LPCWSTR)0x0);
  if ((((pHVar1 == (HMODULE)0x0) || ((short)pHVar1->unused != 0x5a4d)) ||
      (lVar2 = (longlong)pHVar1[0xf].unused, *(int *)((longlong)&pHVar1->unused + lVar2) != 0x4550))
     || ((*(short *)((longlong)&pHVar1[6].unused + lVar2) != 0x20b ||
         (*(uint *)((longlong)&pHVar1[0x21].unused + lVar2) < 0xf)))) {
    bVar3 = false;
  }
  else {
    bVar3 = *(int *)((longlong)&pHVar1[0x3e].unused + lVar2) != 0;
  }
  return bVar3;
}



// Function: FUN_1400067ac @ 1400067ac
// Signature: void FUN_1400067ac(void);


void FUN_1400067ac(void)

{
                    /* WARNING: Could not recover jumptable at 0x0001400067b3. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)&LAB_1400067bc);
  return;
}



// Function: FUN_140006820 @ 140006820
// Signature: void FUN_140006820(void);


void FUN_140006820(void)

{
  longlong *plVar1;
  
  for (plVar1 = &DAT_14000b900; plVar1 < &DAT_14000b900; plVar1 = plVar1 + 1) {
    if (*plVar1 != 0) {
      (*(code *)PTR__guard_dispatch_icall_140008420)();
    }
  }
  return;
}



// Function: FUN_140006898 @ 140006898
// Signature: undefined8 FUN_140006898(void);


/* WARNING: Removing unreachable block (ram,0x000140006989) */
/* WARNING: Removing unreachable block (ram,0x000140006979) */
/* WARNING: Removing unreachable block (ram,0x000140006954) */
/* WARNING: Removing unreachable block (ram,0x0001400068d2) */
/* WARNING: Removing unreachable block (ram,0x0001400068b0) */
/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined8 FUN_140006898(void)

{
  int *piVar1;
  uint *puVar2;
  int *piVar3;
  longlong lVar4;
  uint uVar5;
  byte bVar6;
  ulonglong uVar7;
  uint uVar8;
  uint uVar9;
  uint uVar10;
  ulonglong uVar11;
  uint in_XCR0;
  
  piVar1 = (int *)cpuid_basic_info(0);
  puVar2 = (uint *)cpuid_Version_info(1);
  uVar5 = puVar2[3];
  if ((piVar1[2] == 0x49656e69 && piVar1[3] == 0x6c65746e) && piVar1[1] == 0x756e6547) {
    uVar8 = *puVar2 & 0xfff3ff0;
    _DAT_14000e018 = 0x8000;
    _DAT_14000e020 = 0xffffffffffffffff;
    if ((((uVar8 == 0x106c0) || (uVar8 == 0x20660)) || (uVar8 == 0x20670)) ||
       ((uVar8 - 0x30650 < 0x21 &&
        ((0x100010001U >> ((ulonglong)(uVar8 - 0x30650) & 0x3f) & 1) != 0)))) {
      DAT_14000e3a8 = DAT_14000e3a8 | 1;
    }
  }
  uVar8 = 0;
  uVar9 = 0;
  uVar10 = 0;
  uVar11 = 0;
  if (6 < *piVar1) {
    piVar3 = (int *)cpuid_Extended_Feature_Enumeration_info(7);
    uVar8 = piVar3[1];
    uVar9 = piVar3[2];
    if ((uVar8 >> 9 & 1) != 0) {
      DAT_14000e3a8 = DAT_14000e3a8 | 2;
    }
    if (0 < *piVar3) {
      lVar4 = cpuid_Extended_Feature_Enumeration_info(7);
      uVar10 = *(uint *)(lVar4 + 8);
    }
    if (0x23 < *piVar1) {
      lVar4 = cpuid(0x24);
      uVar11 = (ulonglong)*(uint *)(lVar4 + 4);
    }
  }
  _DAT_14000e010 = 1;
  DAT_14000e014 = 2;
  uVar7 = DAT_14000e008 & 0xfffffffffffffffe;
  if ((uVar5 >> 0x14 & 1) != 0) {
    _DAT_14000e010 = 2;
    DAT_14000e014 = 6;
    uVar7 = DAT_14000e008 & 0xffffffffffffffee;
  }
  DAT_14000e008 = uVar7;
  if ((uVar5 >> 0x1b & 1) != 0) {
    if (((uVar5 >> 0x1c & 1) != 0) && (bVar6 = (byte)in_XCR0, (bVar6 & 6) == 6)) {
      _DAT_14000e010 = 3;
      uVar7 = DAT_14000e008;
      uVar5 = DAT_14000e014 | 8;
      if ((uVar8 & 0x20) != 0) {
        _DAT_14000e010 = 5;
        uVar7 = DAT_14000e008 & 0xfffffffffffffffd;
        uVar5 = DAT_14000e014 | 0x28;
        if (((uVar8 & 0xd0030000) == 0xd0030000) && ((bVar6 & 0xe0) == 0xe0)) {
          DAT_14000e014 = DAT_14000e014 | 0x68;
          _DAT_14000e010 = 6;
          uVar7 = DAT_14000e008 & 0xffffffffffffffd9;
          uVar5 = DAT_14000e014;
        }
      }
      DAT_14000e014 = uVar5;
      DAT_14000e008 = uVar7;
      if ((uVar9 >> 0x17 & 1) != 0) {
        DAT_14000e008 = DAT_14000e008 & 0xfffffffffeffffff;
      }
      if (((uVar10 >> 0x13 & 1) != 0) && ((bVar6 & 0xe0) == 0xe0)) {
        _DAT_14000e3a4 = (uint)uVar11 & 0x400ff;
        DAT_14000e008 = ~((ulonglong)((uint)(uVar11 >> 0x10) & 6) | 0x1000029) & DAT_14000e008;
        if (1 < (byte)_DAT_14000e3a4) {
          DAT_14000e008 = DAT_14000e008 & 0xffffffffffffffbf;
        }
      }
    }
    if (((uVar10 >> 0x15 & 1) != 0) && ((in_XCR0 >> 0x13 & 1) != 0)) {
      DAT_14000e008 = DAT_14000e008 & 0xffffffffffffff7f;
    }
  }
  return 0;
}



// Function: __scrt_is_ucrt_dll_in_use @ 140006b30
// Signature: bool __scrt_is_ucrt_dll_in_use(void);


/* Library Function - Single Match
    __scrt_is_ucrt_dll_in_use
   
   Library: Visual Studio 2019 Release */

bool __scrt_is_ucrt_dll_in_use(void)

{
  return DAT_14000e094 != 0;
}



// Function: FUN_140006b40 @ 140006b40
// Signature: void FUN_140006b40(undefined1 (*param_1) [32],undefined1 (*param_2) [32],undefined1 (*param_3) [16],ulonglong param_4);


void FUN_140006b40(undefined1 (*param_1) [32],undefined1 (*param_2) [32],undefined1 (*param_3) [16],
                  ulonglong param_4)

{
  uint uVar1;
  byte *pbVar2;
  uint uVar3;
  int iVar4;
  char *pcVar5;
  uint *puVar6;
  undefined1 (*pauVar7) [32];
  undefined1 (*pauVar8) [32];
  undefined1 (*pauVar9) [16];
  uint uVar10;
  longlong lVar11;
  uint uVar12;
  ulonglong uVar13;
  uint uVar14;
  undefined1 auVar15 [16];
  undefined1 auVar16 [16];
  undefined1 auVar17 [16];
  undefined1 auVar18 [16];
  uint uVar20;
  uint uVar21;
  undefined1 auVar19 [16];
  undefined1 auStack_d8 [32];
  uint local_b8 [2];
  undefined1 (*local_b0) [32];
  undefined1 (**local_a8 [2]) [32];
  undefined1 local_98 [16];
  undefined1 (*local_88) [16];
  ulonglong local_80;
  undefined1 (*local_78) [32];
  undefined8 uStack_70;
  ulonglong local_68;
  
  local_68 = DAT_14000e040 ^ (ulonglong)auStack_d8;
  local_b0 = param_2;
  if (param_4 != 0) {
    if (param_4 == 1) {
      FUN_140006ef0(param_1,param_2,(*param_3)[0]);
    }
    else {
      uVar13 = (longlong)param_2 - (longlong)param_1;
      if (param_4 <= uVar13) {
        if ((((byte)DAT_14000e014 & 4) == 0) || (uVar13 < 0x10)) {
          pauVar8 = (undefined1 (*) [32])((longlong)param_2 - param_4);
          lVar11 = (longlong)param_3 - (longlong)pauVar8;
          while( true ) {
            if ((*pauVar8)[0] == (*param_3)[0]) {
              pcVar5 = *pauVar8 + 1;
              while (*pcVar5 == pcVar5[lVar11]) {
                pcVar5 = pcVar5 + 1;
                if ((longlong)pcVar5 - (longlong)pauVar8 == param_4) goto LAB_140006ec6;
              }
            }
            if (pauVar8 == param_1) break;
            pauVar8 = (undefined1 (*) [32])(pauVar8[-1] + 0x1f);
            lVar11 = lVar11 + 1;
          }
        }
        else if (param_4 < 0x11) {
          uVar12 = (uint)uVar13 & 0xf;
          uVar14 = (1 << (0x11U - (char)param_4 & 0x1f)) - 1;
          memcpy(&local_78,param_3,param_4);
          auVar17._8_8_ = uStack_70;
          auVar17._0_8_ = local_78;
          pauVar9 = (undefined1 (*) [16])(local_b0[-1] + 0x10);
          auVar15 = pcmpestrm(auVar17,*pauVar9,0xc);
          uVar3 = auVar15._0_4_ & uVar14;
          if (uVar3 == 0) {
            uVar3 = (uint)local_78;
            uVar20 = (uint)((ulonglong)local_78 >> 0x20);
            uVar21 = (uint)((ulonglong)uStack_70 >> 0x20);
            if (pauVar9 != (undefined1 (*) [16])(*param_1 + uVar12)) {
              do {
                pauVar9 = pauVar9 + -1;
                auVar15 = pcmpestrm(auVar17,*pauVar9,0xc);
                uVar10 = auVar15._0_4_;
                if (uVar10 != 0) {
                  local_b8[0] = (uVar14 ^ 0xffff) & uVar10;
                  if (local_b8[0] != 0) {
                    auVar15 = *(undefined1 (*) [16])(&DAT_14000aa80 + -param_4);
                    do {
                      uVar1 = 0x1f;
                      if (local_b8[0] != 0) {
                        for (; local_b8[0] >> uVar1 == 0; uVar1 = uVar1 - 1) {
                        }
                      }
                      puVar6 = (uint *)(*pauVar9 + uVar1);
                      auVar16._0_4_ = *puVar6 ^ uVar3;
                      auVar16._4_4_ = puVar6[1] ^ uVar20;
                      auVar16._8_4_ = puVar6[2] ^ (uint)uStack_70;
                      auVar16._12_4_ = puVar6[3] ^ uVar21;
                      if ((auVar15 & auVar16) == (undefined1  [16])0x0) goto LAB_140006ec6;
                      pbVar2 = (byte *)((longlong)local_b8 + ((longlong)(int)uVar1 >> 3));
                      *pbVar2 = *pbVar2 & ~('\x01' << (uVar1 & 7));
                    } while (local_b8[0] != 0);
                  }
                  uVar10 = uVar10 & uVar14;
                  if (uVar10 != 0) {
                    iVar4 = 0x1f;
                    if (uVar10 != 0) {
                      for (; uVar10 >> iVar4 == 0; iVar4 = iVar4 + -1) {
                      }
                    }
                    goto LAB_140006ec6;
                  }
                }
              } while (pauVar9 != (undefined1 (*) [16])(*param_1 + uVar12));
            }
            if ((ulonglong)uVar12 != 0) {
              auVar17 = pcmpestrm(auVar17,*(undefined1 (*) [16])*param_1,0xc);
              uVar12 = auVar17._0_4_ & (1 << (sbyte)uVar12) - 1U;
              if (uVar12 != 0) {
                local_b8[0] = (uVar14 ^ 0xffff) & uVar12;
                if (local_b8[0] != 0) {
                  auVar17 = *(undefined1 (*) [16])(&DAT_14000aa80 + -param_4);
                  do {
                    uVar10 = 0x1f;
                    if (local_b8[0] != 0) {
                      for (; local_b8[0] >> uVar10 == 0; uVar10 = uVar10 - 1) {
                      }
                    }
                    puVar6 = (uint *)(*param_1 + uVar10);
                    auVar15._0_4_ = *puVar6 ^ uVar3;
                    auVar15._4_4_ = puVar6[1] ^ uVar20;
                    auVar15._8_4_ = puVar6[2] ^ (uint)uStack_70;
                    auVar15._12_4_ = puVar6[3] ^ uVar21;
                    if ((auVar17 & auVar15) == (undefined1  [16])0x0) goto LAB_140006ec6;
                    pbVar2 = (byte *)((longlong)local_b8 + ((longlong)(int)uVar10 >> 3));
                    *pbVar2 = *pbVar2 & ~('\x01' << (uVar10 & 7));
                  } while (local_b8[0] != 0);
                }
                uVar12 = uVar12 & uVar14;
                if ((uVar12 != 0) && (iVar4 = 0x1f, uVar12 != 0)) {
                  for (; uVar12 >> iVar4 == 0; iVar4 = iVar4 + -1) {
                  }
                }
              }
            }
          }
          else {
            iVar4 = 0x1f;
            if (uVar3 != 0) {
              for (; uVar3 >> iVar4 == 0; iVar4 = iVar4 + -1) {
              }
            }
          }
        }
        else {
          auVar17 = *param_3;
          uVar3 = (uint)uVar13 - (int)param_4;
          local_80 = param_4;
          pauVar9 = param_3 + 1;
          local_98 = auVar17;
          local_88 = pauVar9;
          pauVar8 = (undefined1 (*) [32])((longlong)param_2 - param_4);
          local_a8[0] = &local_78;
          auVar18._0_4_ = *(uint *)*pauVar8 ^ auVar17._0_4_;
          auVar18._4_4_ = *(uint *)(*pauVar8 + 4) ^ auVar17._4_4_;
          auVar18._8_4_ = *(uint *)(*pauVar8 + 8) ^ auVar17._8_4_;
          auVar18._12_4_ = *(uint *)(*pauVar8 + 0xc) ^ auVar17._12_4_;
          local_78 = pauVar8;
          if ((auVar18 != (undefined1  [16])0x0) ||
             (iVar4 = memcmp(*pauVar8 + 0x10,pauVar9,param_4 - 0x10), iVar4 != 0)) {
            while (pauVar8 != (undefined1 (*) [32])(*param_1 + (uVar3 & 0xf))) {
              pauVar8 = (undefined1 (*) [32])(pauVar8[-1] + 0x10);
              local_78 = pauVar8;
              auVar15 = pcmpestrm(auVar17,*(undefined1 (*) [16])*pauVar8,0xc);
              if (auVar15._0_4_ != 0) {
                local_b8[0] = auVar15._0_4_;
                do {
                  uVar12 = 0x1f;
                  if (local_b8[0] != 0) {
                    for (; local_b8[0] >> uVar12 == 0; uVar12 = uVar12 - 1) {
                    }
                  }
                  pauVar7 = pauVar8;
                  if (((uVar12 == 0) ||
                      (pauVar7 = (undefined1 (*) [32])(*pauVar8 + uVar12),
                      auVar19._0_4_ = auVar17._0_4_ ^ *(uint *)*pauVar7,
                      auVar19._4_4_ = auVar17._4_4_ ^ *(uint *)(*pauVar7 + 4),
                      auVar19._8_4_ = auVar17._8_4_ ^ *(uint *)(*pauVar7 + 8),
                      auVar19._12_4_ = auVar17._12_4_ ^ *(uint *)(*pauVar7 + 0xc),
                      auVar19 == (undefined1  [16])0x0)) &&
                     (iVar4 = memcmp(*pauVar7 + 0x10,pauVar9,param_4 - 0x10), iVar4 == 0))
                  goto LAB_140006ec6;
                  pbVar2 = (byte *)((longlong)local_b8 + ((longlong)(int)uVar12 >> 3));
                  *pbVar2 = *pbVar2 & ~('\x01' << (uVar12 & 7));
                } while (local_b8[0] != 0);
              }
            }
            uVar3 = uVar3 & 0xf;
            if (uVar3 != 0) {
              auVar17 = pcmpestrm(auVar17,*(undefined1 (*) [16])*param_1,0xc);
              uVar3 = auVar17._0_4_ & (1 << (sbyte)uVar3) - 1U;
              local_78 = param_1;
              if (uVar3 != 0) {
                FUN_140007490(local_a8,uVar3);
              }
            }
          }
        }
      }
    }
  }
LAB_140006ec6:
  FUN_140007700(local_68 ^ (ulonglong)auStack_d8);
  return;
}



// Function: FUN_140006ef0 @ 140006ef0
// Signature: undefined1 (*) [32]FUN_140006ef0(undefined1 (*param_1) [32],undefined1 (*param_2) [32],byte param_3);


undefined1 (*) [32]
FUN_140006ef0(undefined1 (*param_1) [32],undefined1 (*param_2) [32],byte param_3)

{
  ushort uVar1;
  undefined1 auVar2 [32];
  uint uVar3;
  undefined1 (*pauVar4) [32];
  undefined1 (*pauVar5) [32];
  undefined1 (*pauVar6) [32];
  undefined1 (*pauVar7) [32];
  undefined1 (*pauVar8) [32];
  undefined1 (*pauVar9) [32];
  undefined1 (*pauVar10) [32];
  undefined1 (*pauVar11) [32];
  undefined1 (*pauVar12) [32];
  undefined1 (*pauVar13) [32];
  undefined1 (*pauVar14) [32];
  undefined1 (*pauVar15) [32];
  undefined1 (*pauVar16) [32];
  undefined1 (*pauVar17) [32];
  undefined1 (*pauVar18) [32];
  undefined1 (*pauVar19) [32];
  undefined1 (*pauVar20) [32];
  ulonglong uVar21;
  ulonglong uVar22;
  undefined1 auVar23 [16];
  undefined1 auVar24 [16];
  undefined1 auVar25 [32];
  
  uVar21 = (longlong)param_2 - (longlong)param_1;
  pauVar20 = param_2;
  if (((uVar21 & 0xffffffffffffffe0) == 0) || ((DAT_14000e014 & 0x20) == 0)) {
    if (((uVar21 & 0xfffffffffffffff0) != 0) && ((DAT_14000e014 & 4) != 0)) {
      auVar24 = pshufb(ZEXT116(param_3),ZEXT816(0));
      do {
        pauVar4 = pauVar20 + -1;
        pauVar5 = pauVar20 + -1;
        pauVar6 = pauVar20 + -1;
        pauVar7 = pauVar20 + -1;
        pauVar8 = pauVar20 + -1;
        pauVar9 = pauVar20 + -1;
        pauVar10 = pauVar20 + -1;
        pauVar11 = pauVar20 + -1;
        pauVar12 = pauVar20 + -1;
        pauVar13 = pauVar20 + -1;
        pauVar14 = pauVar20 + -1;
        pauVar15 = pauVar20 + -1;
        pauVar16 = pauVar20 + -1;
        pauVar17 = pauVar20 + -1;
        pauVar18 = pauVar20 + -1;
        pauVar19 = pauVar20 + -1;
        pauVar20 = (undefined1 (*) [32])(pauVar20[-1] + 0x10);
        auVar23[0] = -((*pauVar4)[0x10] == auVar24[0]);
        auVar23[1] = -((*pauVar5)[0x11] == auVar24[1]);
        auVar23[2] = -((*pauVar6)[0x12] == auVar24[2]);
        auVar23[3] = -((*pauVar7)[0x13] == auVar24[3]);
        auVar23[4] = -((*pauVar8)[0x14] == auVar24[4]);
        auVar23[5] = -((*pauVar9)[0x15] == auVar24[5]);
        auVar23[6] = -((*pauVar10)[0x16] == auVar24[6]);
        auVar23[7] = -((*pauVar11)[0x17] == auVar24[7]);
        auVar23[8] = -((*pauVar12)[0x18] == auVar24[8]);
        auVar23[9] = -((*pauVar13)[0x19] == auVar24[9]);
        auVar23[10] = -((*pauVar14)[0x1a] == auVar24[10]);
        auVar23[0xb] = -((*pauVar15)[0x1b] == auVar24[0xb]);
        auVar23[0xc] = -((*pauVar16)[0x1c] == auVar24[0xc]);
        auVar23[0xd] = -((*pauVar17)[0x1d] == auVar24[0xd]);
        auVar23[0xe] = -((*pauVar18)[0x1e] == auVar24[0xe]);
        auVar23[0xf] = -((*pauVar19)[0x1f] == auVar24[0xf]);
        uVar1 = (ushort)(SUB161(auVar23 >> 7,0) & 1) | (ushort)(SUB161(auVar23 >> 0xf,0) & 1) << 1 |
                (ushort)(SUB161(auVar23 >> 0x17,0) & 1) << 2 |
                (ushort)(SUB161(auVar23 >> 0x1f,0) & 1) << 3 |
                (ushort)(SUB161(auVar23 >> 0x27,0) & 1) << 4 |
                (ushort)(SUB161(auVar23 >> 0x2f,0) & 1) << 5 |
                (ushort)(SUB161(auVar23 >> 0x37,0) & 1) << 6 |
                (ushort)(SUB161(auVar23 >> 0x3f,0) & 1) << 7 |
                (ushort)(SUB161(auVar23 >> 0x47,0) & 1) << 8 |
                (ushort)(SUB161(auVar23 >> 0x4f,0) & 1) << 9 |
                (ushort)(SUB161(auVar23 >> 0x57,0) & 1) << 10 |
                (ushort)(SUB161(auVar23 >> 0x5f,0) & 1) << 0xb |
                (ushort)(SUB161(auVar23 >> 0x67,0) & 1) << 0xc |
                (ushort)(SUB161(auVar23 >> 0x6f,0) & 1) << 0xd |
                (ushort)(SUB161(auVar23 >> 0x77,0) & 1) << 0xe | (ushort)(auVar23[0xf] >> 7) << 0xf;
        if (uVar1 != 0) {
          uVar3 = 0x1f;
          if (uVar1 != 0) {
            for (; uVar1 >> uVar3 == 0; uVar3 = uVar3 - 1) {
            }
          }
          return (undefined1 (*) [32])(*pauVar20 + uVar3);
        }
      } while (pauVar20 != (undefined1 (*) [32])((longlong)param_2 - (uVar21 & 0xfffffffffffffff0)))
      ;
    }
  }
  else {
    auVar24 = vpshufb_avx(ZEXT416((uint)(int)(char)param_3),(undefined1  [16])0x0);
    auVar25._16_16_ = auVar24;
    auVar25._0_16_ = auVar24;
    do {
      auVar2 = vpcmpeqb_avx2(auVar25,pauVar20[-1]);
      pauVar20 = pauVar20 + -1;
      uVar3 = (uint)(SUB321(auVar2 >> 7,0) & 1) | (uint)(SUB321(auVar2 >> 0xf,0) & 1) << 1 |
              (uint)(SUB321(auVar2 >> 0x17,0) & 1) << 2 | (uint)(SUB321(auVar2 >> 0x1f,0) & 1) << 3
              | (uint)(SUB321(auVar2 >> 0x27,0) & 1) << 4 |
              (uint)(SUB321(auVar2 >> 0x2f,0) & 1) << 5 | (uint)(SUB321(auVar2 >> 0x37,0) & 1) << 6
              | (uint)(SUB321(auVar2 >> 0x3f,0) & 1) << 7 |
              (uint)(SUB321(auVar2 >> 0x47,0) & 1) << 8 | (uint)(SUB321(auVar2 >> 0x4f,0) & 1) << 9
              | (uint)(SUB321(auVar2 >> 0x57,0) & 1) << 10 |
              (uint)(SUB321(auVar2 >> 0x5f,0) & 1) << 0xb |
              (uint)(SUB321(auVar2 >> 0x67,0) & 1) << 0xc |
              (uint)(SUB321(auVar2 >> 0x6f,0) & 1) << 0xd |
              (uint)(SUB321(auVar2 >> 0x77,0) & 1) << 0xe | (uint)SUB321(auVar2 >> 0x7f,0) << 0xf |
              (uint)(SUB321(auVar2 >> 0x87,0) & 1) << 0x10 |
              (uint)(SUB321(auVar2 >> 0x8f,0) & 1) << 0x11 |
              (uint)(SUB321(auVar2 >> 0x97,0) & 1) << 0x12 |
              (uint)(SUB321(auVar2 >> 0x9f,0) & 1) << 0x13 |
              (uint)(SUB321(auVar2 >> 0xa7,0) & 1) << 0x14 |
              (uint)(SUB321(auVar2 >> 0xaf,0) & 1) << 0x15 |
              (uint)(SUB321(auVar2 >> 0xb7,0) & 1) << 0x16 | (uint)SUB321(auVar2 >> 0xbf,0) << 0x17
              | (uint)(SUB321(auVar2 >> 199,0) & 1) << 0x18 |
              (uint)(SUB321(auVar2 >> 0xcf,0) & 1) << 0x19 |
              (uint)(SUB321(auVar2 >> 0xd7,0) & 1) << 0x1a |
              (uint)(SUB321(auVar2 >> 0xdf,0) & 1) << 0x1b |
              (uint)(SUB321(auVar2 >> 0xe7,0) & 1) << 0x1c |
              (uint)(SUB321(auVar2 >> 0xef,0) & 1) << 0x1d |
              (uint)(SUB321(auVar2 >> 0xf7,0) & 1) << 0x1e | (uint)(byte)(auVar2[0x1f] >> 7) << 0x1f
      ;
      if (uVar3 != 0) {
        return (undefined1 (*) [32])(*pauVar20 + (0x1f - LZCOUNT(uVar3)));
      }
    } while (pauVar20 != (undefined1 (*) [32])((longlong)param_2 - (uVar21 & 0xffffffffffffffe0)));
    uVar22 = (ulonglong)((uint)uVar21 & 0x1c);
    if ((uVar21 & 0x1c) != 0) {
      pauVar20 = (undefined1 (*) [32])((longlong)pauVar20 - uVar22);
      auVar2 = vpmaskmovd_avx2(*(undefined1 (*) [32])(&DAT_14000aa50 + -uVar22),*pauVar20);
      auVar25 = vpcmpeqb_avx2(auVar2,auVar25);
      auVar25 = vpand_avx2(auVar25,*(undefined1 (*) [32])(&DAT_14000aa50 + -uVar22));
      uVar3 = (uint)(SUB321(auVar25 >> 7,0) & 1) | (uint)(SUB321(auVar25 >> 0xf,0) & 1) << 1 |
              (uint)(SUB321(auVar25 >> 0x17,0) & 1) << 2 |
              (uint)(SUB321(auVar25 >> 0x1f,0) & 1) << 3 |
              (uint)(SUB321(auVar25 >> 0x27,0) & 1) << 4 |
              (uint)(SUB321(auVar25 >> 0x2f,0) & 1) << 5 |
              (uint)(SUB321(auVar25 >> 0x37,0) & 1) << 6 |
              (uint)(SUB321(auVar25 >> 0x3f,0) & 1) << 7 |
              (uint)(SUB321(auVar25 >> 0x47,0) & 1) << 8 |
              (uint)(SUB321(auVar25 >> 0x4f,0) & 1) << 9 |
              (uint)(SUB321(auVar25 >> 0x57,0) & 1) << 10 |
              (uint)(SUB321(auVar25 >> 0x5f,0) & 1) << 0xb |
              (uint)(SUB321(auVar25 >> 0x67,0) & 1) << 0xc |
              (uint)(SUB321(auVar25 >> 0x6f,0) & 1) << 0xd |
              (uint)(SUB321(auVar25 >> 0x77,0) & 1) << 0xe | (uint)SUB321(auVar25 >> 0x7f,0) << 0xf
              | (uint)(SUB321(auVar25 >> 0x87,0) & 1) << 0x10 |
              (uint)(SUB321(auVar25 >> 0x8f,0) & 1) << 0x11 |
              (uint)(SUB321(auVar25 >> 0x97,0) & 1) << 0x12 |
              (uint)(SUB321(auVar25 >> 0x9f,0) & 1) << 0x13 |
              (uint)(SUB321(auVar25 >> 0xa7,0) & 1) << 0x14 |
              (uint)(SUB321(auVar25 >> 0xaf,0) & 1) << 0x15 |
              (uint)(SUB321(auVar25 >> 0xb7,0) & 1) << 0x16 |
              (uint)SUB321(auVar25 >> 0xbf,0) << 0x17 | (uint)(SUB321(auVar25 >> 199,0) & 1) << 0x18
              | (uint)(SUB321(auVar25 >> 0xcf,0) & 1) << 0x19 |
              (uint)(SUB321(auVar25 >> 0xd7,0) & 1) << 0x1a |
              (uint)(SUB321(auVar25 >> 0xdf,0) & 1) << 0x1b |
              (uint)(SUB321(auVar25 >> 0xe7,0) & 1) << 0x1c |
              (uint)(SUB321(auVar25 >> 0xef,0) & 1) << 0x1d |
              (uint)(SUB321(auVar25 >> 0xf7,0) & 1) << 0x1e |
              (uint)(byte)(auVar25[0x1f] >> 7) << 0x1f;
      if (uVar3 != 0) {
        return (undefined1 (*) [32])(*pauVar20 + (0x1f - LZCOUNT(uVar3)));
      }
    }
  }
  do {
    if (pauVar20 == param_1) {
      return param_2;
    }
    pauVar20 = (undefined1 (*) [32])(pauVar20[-1] + 0x1f);
  } while ((*pauVar20)[0] != param_3);
  return pauVar20;
}



// Function: FUN_140007040 @ 140007040
// Signature: undefined1 (*) [32]FUN_140007040(undefined1 (*param_1) [32],undefined1 (*param_2) [32],short param_3);


undefined1 (*) [32]
FUN_140007040(undefined1 (*param_1) [32],undefined1 (*param_2) [32],short param_3)

{
  ushort uVar1;
  undefined1 auVar2 [32];
  uint uVar3;
  undefined1 (*pauVar4) [32];
  undefined1 (*pauVar5) [32];
  undefined1 (*pauVar6) [32];
  undefined1 (*pauVar7) [32];
  undefined1 (*pauVar8) [32];
  undefined1 (*pauVar9) [32];
  undefined1 (*pauVar10) [32];
  undefined1 (*pauVar11) [32];
  undefined1 (*pauVar12) [32];
  ulonglong uVar13;
  ulonglong uVar14;
  undefined1 auVar15 [16];
  undefined1 auVar16 [32];
  
  uVar13 = (longlong)param_2 - (longlong)param_1;
  pauVar12 = param_2;
  if (((uVar13 & 0xffffffffffffffe0) == 0) || ((DAT_14000e014 & 0x20) == 0)) {
    if (((uVar13 & 0xfffffffffffffff0) != 0) && ((DAT_14000e014 & 4) != 0)) {
      do {
        pauVar4 = pauVar12 + -1;
        pauVar5 = pauVar12 + -1;
        pauVar6 = pauVar12 + -1;
        pauVar7 = pauVar12 + -1;
        pauVar8 = pauVar12 + -1;
        pauVar9 = pauVar12 + -1;
        pauVar10 = pauVar12 + -1;
        pauVar11 = pauVar12 + -1;
        pauVar12 = (undefined1 (*) [32])(pauVar12[-1] + 0x10);
        auVar15._0_2_ = -(ushort)(*(short *)(*pauVar4 + 0x10) == param_3);
        auVar15._2_2_ = -(ushort)(*(short *)(*pauVar5 + 0x12) == param_3);
        auVar15._4_2_ = -(ushort)(*(short *)(*pauVar6 + 0x14) == param_3);
        auVar15._6_2_ = -(ushort)(*(short *)(*pauVar7 + 0x16) == param_3);
        auVar15._8_2_ = -(ushort)(*(short *)(*pauVar8 + 0x18) == param_3);
        auVar15._10_2_ = -(ushort)(*(short *)(*pauVar9 + 0x1a) == param_3);
        auVar15._12_2_ = -(ushort)(*(short *)(*pauVar10 + 0x1c) == param_3);
        auVar15._14_2_ = -(ushort)(*(short *)(*pauVar11 + 0x1e) == param_3);
        uVar1 = (ushort)(SUB161(auVar15 >> 7,0) & 1) | (ushort)(SUB161(auVar15 >> 0xf,0) & 1) << 1 |
                (ushort)(SUB161(auVar15 >> 0x17,0) & 1) << 2 |
                (ushort)(SUB161(auVar15 >> 0x1f,0) & 1) << 3 |
                (ushort)(SUB161(auVar15 >> 0x27,0) & 1) << 4 |
                (ushort)(SUB161(auVar15 >> 0x2f,0) & 1) << 5 |
                (ushort)(SUB161(auVar15 >> 0x37,0) & 1) << 6 |
                (ushort)(SUB161(auVar15 >> 0x3f,0) & 1) << 7 |
                (ushort)(SUB161(auVar15 >> 0x47,0) & 1) << 8 |
                (ushort)(SUB161(auVar15 >> 0x4f,0) & 1) << 9 |
                (ushort)(SUB161(auVar15 >> 0x57,0) & 1) << 10 |
                (ushort)(SUB161(auVar15 >> 0x5f,0) & 1) << 0xb |
                (ushort)(SUB161(auVar15 >> 0x67,0) & 1) << 0xc |
                (ushort)(SUB161(auVar15 >> 0x6f,0) & 1) << 0xd |
                (ushort)((byte)(auVar15._14_2_ >> 7) & 1) << 0xe | auVar15._14_2_ & 0x8000;
        if (uVar1 != 0) {
          uVar3 = 0x1f;
          if (uVar1 != 0) {
            for (; uVar1 >> uVar3 == 0; uVar3 = uVar3 - 1) {
            }
          }
          return (undefined1 (*) [32])(pauVar12[-1] + (ulonglong)uVar3 + 0x1f);
        }
      } while (pauVar12 != (undefined1 (*) [32])((longlong)param_2 - (uVar13 & 0xfffffffffffffff0)))
      ;
    }
  }
  else {
    auVar15 = vpunpcklwd_avx(ZEXT416((uint)(int)param_3),ZEXT416((uint)(int)param_3));
    auVar15 = vpshufd_avx(auVar15,0);
    auVar16._16_16_ = auVar15;
    auVar16._0_16_ = auVar15;
    do {
      auVar2 = vpcmpeqw_avx2(auVar16,pauVar12[-1]);
      pauVar12 = pauVar12 + -1;
      uVar3 = (uint)(SUB321(auVar2 >> 7,0) & 1) | (uint)(SUB321(auVar2 >> 0xf,0) & 1) << 1 |
              (uint)(SUB321(auVar2 >> 0x17,0) & 1) << 2 | (uint)(SUB321(auVar2 >> 0x1f,0) & 1) << 3
              | (uint)(SUB321(auVar2 >> 0x27,0) & 1) << 4 |
              (uint)(SUB321(auVar2 >> 0x2f,0) & 1) << 5 | (uint)(SUB321(auVar2 >> 0x37,0) & 1) << 6
              | (uint)(SUB321(auVar2 >> 0x3f,0) & 1) << 7 |
              (uint)(SUB321(auVar2 >> 0x47,0) & 1) << 8 | (uint)(SUB321(auVar2 >> 0x4f,0) & 1) << 9
              | (uint)(SUB321(auVar2 >> 0x57,0) & 1) << 10 |
              (uint)(SUB321(auVar2 >> 0x5f,0) & 1) << 0xb |
              (uint)(SUB321(auVar2 >> 0x67,0) & 1) << 0xc |
              (uint)(SUB321(auVar2 >> 0x6f,0) & 1) << 0xd |
              (uint)(SUB321(auVar2 >> 0x77,0) & 1) << 0xe | (uint)SUB321(auVar2 >> 0x7f,0) << 0xf |
              (uint)(SUB321(auVar2 >> 0x87,0) & 1) << 0x10 |
              (uint)(SUB321(auVar2 >> 0x8f,0) & 1) << 0x11 |
              (uint)(SUB321(auVar2 >> 0x97,0) & 1) << 0x12 |
              (uint)(SUB321(auVar2 >> 0x9f,0) & 1) << 0x13 |
              (uint)(SUB321(auVar2 >> 0xa7,0) & 1) << 0x14 |
              (uint)(SUB321(auVar2 >> 0xaf,0) & 1) << 0x15 |
              (uint)(SUB321(auVar2 >> 0xb7,0) & 1) << 0x16 | (uint)SUB321(auVar2 >> 0xbf,0) << 0x17
              | (uint)(SUB321(auVar2 >> 199,0) & 1) << 0x18 |
              (uint)(SUB321(auVar2 >> 0xcf,0) & 1) << 0x19 |
              (uint)(SUB321(auVar2 >> 0xd7,0) & 1) << 0x1a |
              (uint)(SUB321(auVar2 >> 0xdf,0) & 1) << 0x1b |
              (uint)(SUB321(auVar2 >> 0xe7,0) & 1) << 0x1c |
              (uint)(SUB321(auVar2 >> 0xef,0) & 1) << 0x1d |
              (uint)(SUB321(auVar2 >> 0xf7,0) & 1) << 0x1e | (uint)(byte)(auVar2[0x1f] >> 7) << 0x1f
      ;
      if (uVar3 != 0) {
        return (undefined1 (*) [32])(pauVar12[-1] + (ulonglong)(0x1f - LZCOUNT(uVar3)) + 0x1f);
      }
    } while (pauVar12 != (undefined1 (*) [32])((longlong)param_2 - (uVar13 & 0xffffffffffffffe0)));
    uVar14 = (ulonglong)((uint)uVar13 & 0x1c);
    if ((uVar13 & 0x1c) != 0) {
      pauVar12 = (undefined1 (*) [32])((longlong)pauVar12 - uVar14);
      auVar2 = vpmaskmovd_avx2(*(undefined1 (*) [32])(&DAT_14000aa50 + -uVar14),*pauVar12);
      auVar16 = vpcmpeqw_avx2(auVar2,auVar16);
      auVar16 = vpand_avx2(auVar16,*(undefined1 (*) [32])(&DAT_14000aa50 + -uVar14));
      uVar3 = (uint)(SUB321(auVar16 >> 7,0) & 1) | (uint)(SUB321(auVar16 >> 0xf,0) & 1) << 1 |
              (uint)(SUB321(auVar16 >> 0x17,0) & 1) << 2 |
              (uint)(SUB321(auVar16 >> 0x1f,0) & 1) << 3 |
              (uint)(SUB321(auVar16 >> 0x27,0) & 1) << 4 |
              (uint)(SUB321(auVar16 >> 0x2f,0) & 1) << 5 |
              (uint)(SUB321(auVar16 >> 0x37,0) & 1) << 6 |
              (uint)(SUB321(auVar16 >> 0x3f,0) & 1) << 7 |
              (uint)(SUB321(auVar16 >> 0x47,0) & 1) << 8 |
              (uint)(SUB321(auVar16 >> 0x4f,0) & 1) << 9 |
              (uint)(SUB321(auVar16 >> 0x57,0) & 1) << 10 |
              (uint)(SUB321(auVar16 >> 0x5f,0) & 1) << 0xb |
              (uint)(SUB321(auVar16 >> 0x67,0) & 1) << 0xc |
              (uint)(SUB321(auVar16 >> 0x6f,0) & 1) << 0xd |
              (uint)(SUB321(auVar16 >> 0x77,0) & 1) << 0xe | (uint)SUB321(auVar16 >> 0x7f,0) << 0xf
              | (uint)(SUB321(auVar16 >> 0x87,0) & 1) << 0x10 |
              (uint)(SUB321(auVar16 >> 0x8f,0) & 1) << 0x11 |
              (uint)(SUB321(auVar16 >> 0x97,0) & 1) << 0x12 |
              (uint)(SUB321(auVar16 >> 0x9f,0) & 1) << 0x13 |
              (uint)(SUB321(auVar16 >> 0xa7,0) & 1) << 0x14 |
              (uint)(SUB321(auVar16 >> 0xaf,0) & 1) << 0x15 |
              (uint)(SUB321(auVar16 >> 0xb7,0) & 1) << 0x16 |
              (uint)SUB321(auVar16 >> 0xbf,0) << 0x17 | (uint)(SUB321(auVar16 >> 199,0) & 1) << 0x18
              | (uint)(SUB321(auVar16 >> 0xcf,0) & 1) << 0x19 |
              (uint)(SUB321(auVar16 >> 0xd7,0) & 1) << 0x1a |
              (uint)(SUB321(auVar16 >> 0xdf,0) & 1) << 0x1b |
              (uint)(SUB321(auVar16 >> 0xe7,0) & 1) << 0x1c |
              (uint)(SUB321(auVar16 >> 0xef,0) & 1) << 0x1d |
              (uint)(SUB321(auVar16 >> 0xf7,0) & 1) << 0x1e |
              (uint)(byte)(auVar16[0x1f] >> 7) << 0x1f;
      if (uVar3 != 0) {
        return (undefined1 (*) [32])(pauVar12[-1] + (ulonglong)(0x1f - LZCOUNT(uVar3)) + 0x1f);
      }
    }
  }
  do {
    if (pauVar12 == param_1) {
      return param_2;
    }
    pauVar12 = (undefined1 (*) [32])(pauVar12[-1] + 0x1e);
  } while (*(short *)*pauVar12 != param_3);
  return pauVar12;
}



// Function: FUN_1400071a0 @ 1400071a0
// Signature: undefined1 (*) [32]FUN_1400071a0(undefined1 (*param_1) [32],undefined1 (*param_2) [32],byte param_3);


undefined1 (*) [32]
FUN_1400071a0(undefined1 (*param_1) [32],undefined1 (*param_2) [32],byte param_3)

{
  ushort uVar1;
  undefined1 auVar2 [32];
  undefined1 *puVar3;
  uint uVar4;
  uint uVar5;
  ulonglong uVar6;
  undefined1 auVar7 [16];
  undefined1 auVar8 [16];
  undefined1 auVar9 [32];
  
  uVar6 = (longlong)param_2 - (longlong)param_1;
  if (((uVar6 & 0xffffffffffffffe0) == 0) || ((DAT_14000e014 & 0x20) == 0)) {
    if (((uVar6 & 0xfffffffffffffff0) != 0) && ((DAT_14000e014 & 4) != 0)) {
      puVar3 = *param_1;
      auVar8 = pshufb(ZEXT116(param_3),ZEXT816(0));
      do {
        auVar7[0] = -((*param_1)[0] == auVar8[0]);
        auVar7[1] = -((*param_1)[1] == auVar8[1]);
        auVar7[2] = -((*param_1)[2] == auVar8[2]);
        auVar7[3] = -((*param_1)[3] == auVar8[3]);
        auVar7[4] = -((*param_1)[4] == auVar8[4]);
        auVar7[5] = -((*param_1)[5] == auVar8[5]);
        auVar7[6] = -((*param_1)[6] == auVar8[6]);
        auVar7[7] = -((*param_1)[7] == auVar8[7]);
        auVar7[8] = -((*param_1)[8] == auVar8[8]);
        auVar7[9] = -((*param_1)[9] == auVar8[9]);
        auVar7[10] = -((*param_1)[10] == auVar8[10]);
        auVar7[0xb] = -((*param_1)[0xb] == auVar8[0xb]);
        auVar7[0xc] = -((*param_1)[0xc] == auVar8[0xc]);
        auVar7[0xd] = -((*param_1)[0xd] == auVar8[0xd]);
        auVar7[0xe] = -((*param_1)[0xe] == auVar8[0xe]);
        auVar7[0xf] = -((*param_1)[0xf] == auVar8[0xf]);
        uVar1 = (ushort)(SUB161(auVar7 >> 7,0) & 1) | (ushort)(SUB161(auVar7 >> 0xf,0) & 1) << 1 |
                (ushort)(SUB161(auVar7 >> 0x17,0) & 1) << 2 |
                (ushort)(SUB161(auVar7 >> 0x1f,0) & 1) << 3 |
                (ushort)(SUB161(auVar7 >> 0x27,0) & 1) << 4 |
                (ushort)(SUB161(auVar7 >> 0x2f,0) & 1) << 5 |
                (ushort)(SUB161(auVar7 >> 0x37,0) & 1) << 6 |
                (ushort)(SUB161(auVar7 >> 0x3f,0) & 1) << 7 |
                (ushort)(SUB161(auVar7 >> 0x47,0) & 1) << 8 |
                (ushort)(SUB161(auVar7 >> 0x4f,0) & 1) << 9 |
                (ushort)(SUB161(auVar7 >> 0x57,0) & 1) << 10 |
                (ushort)(SUB161(auVar7 >> 0x5f,0) & 1) << 0xb |
                (ushort)(SUB161(auVar7 >> 0x67,0) & 1) << 0xc |
                (ushort)(SUB161(auVar7 >> 0x6f,0) & 1) << 0xd |
                (ushort)(SUB161(auVar7 >> 0x77,0) & 1) << 0xe | (ushort)(auVar7[0xf] >> 7) << 0xf;
        if (uVar1 != 0) {
          uVar4 = 0;
          if (uVar1 != 0) {
            for (; (uVar1 >> uVar4 & 1) == 0; uVar4 = uVar4 + 1) {
            }
          }
          return (undefined1 (*) [32])(*param_1 + uVar4);
        }
        param_1 = (undefined1 (*) [32])(*param_1 + 0x10);
      } while (param_1 != (undefined1 (*) [32])(puVar3 + (uVar6 & 0xfffffffffffffff0)));
    }
  }
  else {
    puVar3 = *param_1;
    auVar8 = vpshufb_avx(ZEXT416((uint)(int)(char)param_3),(undefined1  [16])0x0);
    auVar9._16_16_ = auVar8;
    auVar9._0_16_ = auVar8;
    do {
      auVar2 = vpcmpeqb_avx2(auVar9,*param_1);
      uVar4 = (uint)(SUB321(auVar2 >> 7,0) & 1) | (uint)(SUB321(auVar2 >> 0xf,0) & 1) << 1 |
              (uint)(SUB321(auVar2 >> 0x17,0) & 1) << 2 | (uint)(SUB321(auVar2 >> 0x1f,0) & 1) << 3
              | (uint)(SUB321(auVar2 >> 0x27,0) & 1) << 4 |
              (uint)(SUB321(auVar2 >> 0x2f,0) & 1) << 5 | (uint)(SUB321(auVar2 >> 0x37,0) & 1) << 6
              | (uint)(SUB321(auVar2 >> 0x3f,0) & 1) << 7 |
              (uint)(SUB321(auVar2 >> 0x47,0) & 1) << 8 | (uint)(SUB321(auVar2 >> 0x4f,0) & 1) << 9
              | (uint)(SUB321(auVar2 >> 0x57,0) & 1) << 10 |
              (uint)(SUB321(auVar2 >> 0x5f,0) & 1) << 0xb |
              (uint)(SUB321(auVar2 >> 0x67,0) & 1) << 0xc |
              (uint)(SUB321(auVar2 >> 0x6f,0) & 1) << 0xd |
              (uint)(SUB321(auVar2 >> 0x77,0) & 1) << 0xe | (uint)SUB321(auVar2 >> 0x7f,0) << 0xf |
              (uint)(SUB321(auVar2 >> 0x87,0) & 1) << 0x10 |
              (uint)(SUB321(auVar2 >> 0x8f,0) & 1) << 0x11 |
              (uint)(SUB321(auVar2 >> 0x97,0) & 1) << 0x12 |
              (uint)(SUB321(auVar2 >> 0x9f,0) & 1) << 0x13 |
              (uint)(SUB321(auVar2 >> 0xa7,0) & 1) << 0x14 |
              (uint)(SUB321(auVar2 >> 0xaf,0) & 1) << 0x15 |
              (uint)(SUB321(auVar2 >> 0xb7,0) & 1) << 0x16 | (uint)SUB321(auVar2 >> 0xbf,0) << 0x17
              | (uint)(SUB321(auVar2 >> 199,0) & 1) << 0x18 |
              (uint)(SUB321(auVar2 >> 0xcf,0) & 1) << 0x19 |
              (uint)(SUB321(auVar2 >> 0xd7,0) & 1) << 0x1a |
              (uint)(SUB321(auVar2 >> 0xdf,0) & 1) << 0x1b |
              (uint)(SUB321(auVar2 >> 0xe7,0) & 1) << 0x1c |
              (uint)(SUB321(auVar2 >> 0xef,0) & 1) << 0x1d |
              (uint)(SUB321(auVar2 >> 0xf7,0) & 1) << 0x1e | (uint)(byte)(auVar2[0x1f] >> 7) << 0x1f
      ;
      if (uVar4 != 0) goto LAB_140007241;
      param_1 = param_1 + 1;
    } while (param_1 != (undefined1 (*) [32])(puVar3 + (uVar6 & 0xffffffffffffffe0)));
    uVar5 = (uint)uVar6 & 0x1c;
    if ((uVar6 & 0x1c) != 0) {
      auVar2 = vpmaskmovd_avx2(*(undefined1 (*) [32])(&DAT_14000aa50 + -(ulonglong)uVar5),*param_1);
      auVar9 = vpcmpeqb_avx2(auVar2,auVar9);
      auVar9 = vpand_avx2(auVar9,*(undefined1 (*) [32])(&DAT_14000aa50 + -(ulonglong)uVar5));
      uVar4 = (uint)(SUB321(auVar9 >> 7,0) & 1) | (uint)(SUB321(auVar9 >> 0xf,0) & 1) << 1 |
              (uint)(SUB321(auVar9 >> 0x17,0) & 1) << 2 | (uint)(SUB321(auVar9 >> 0x1f,0) & 1) << 3
              | (uint)(SUB321(auVar9 >> 0x27,0) & 1) << 4 |
              (uint)(SUB321(auVar9 >> 0x2f,0) & 1) << 5 | (uint)(SUB321(auVar9 >> 0x37,0) & 1) << 6
              | (uint)(SUB321(auVar9 >> 0x3f,0) & 1) << 7 |
              (uint)(SUB321(auVar9 >> 0x47,0) & 1) << 8 | (uint)(SUB321(auVar9 >> 0x4f,0) & 1) << 9
              | (uint)(SUB321(auVar9 >> 0x57,0) & 1) << 10 |
              (uint)(SUB321(auVar9 >> 0x5f,0) & 1) << 0xb |
              (uint)(SUB321(auVar9 >> 0x67,0) & 1) << 0xc |
              (uint)(SUB321(auVar9 >> 0x6f,0) & 1) << 0xd |
              (uint)(SUB321(auVar9 >> 0x77,0) & 1) << 0xe | (uint)SUB321(auVar9 >> 0x7f,0) << 0xf |
              (uint)(SUB321(auVar9 >> 0x87,0) & 1) << 0x10 |
              (uint)(SUB321(auVar9 >> 0x8f,0) & 1) << 0x11 |
              (uint)(SUB321(auVar9 >> 0x97,0) & 1) << 0x12 |
              (uint)(SUB321(auVar9 >> 0x9f,0) & 1) << 0x13 |
              (uint)(SUB321(auVar9 >> 0xa7,0) & 1) << 0x14 |
              (uint)(SUB321(auVar9 >> 0xaf,0) & 1) << 0x15 |
              (uint)(SUB321(auVar9 >> 0xb7,0) & 1) << 0x16 | (uint)SUB321(auVar9 >> 0xbf,0) << 0x17
              | (uint)(SUB321(auVar9 >> 199,0) & 1) << 0x18 |
              (uint)(SUB321(auVar9 >> 0xcf,0) & 1) << 0x19 |
              (uint)(SUB321(auVar9 >> 0xd7,0) & 1) << 0x1a |
              (uint)(SUB321(auVar9 >> 0xdf,0) & 1) << 0x1b |
              (uint)(SUB321(auVar9 >> 0xe7,0) & 1) << 0x1c |
              (uint)(SUB321(auVar9 >> 0xef,0) & 1) << 0x1d |
              (uint)(SUB321(auVar9 >> 0xf7,0) & 1) << 0x1e | (uint)(byte)(auVar9[0x1f] >> 7) << 0x1f
      ;
      if (uVar4 != 0) {
LAB_140007241:
        uVar5 = 0;
        for (; (uVar4 & 1) == 0; uVar4 = uVar4 >> 1 | 0x80000000) {
          uVar5 = uVar5 + 1;
        }
        return (undefined1 (*) [32])(*param_1 + uVar5);
      }
      param_1 = (undefined1 (*) [32])(*param_1 + uVar5);
    }
  }
  for (; (param_1 != param_2 && ((*param_1)[0] != param_3));
      param_1 = (undefined1 (*) [32])(*param_1 + 1)) {
  }
  return param_1;
}



// Function: FUN_1400072a0 @ 1400072a0
// Signature: void FUN_1400072a0(undefined1 (*param_1) [32],undefined1 (*param_2) [32],undefined1 (*param_3) [16],ulonglong param_4);


void FUN_1400072a0(undefined1 (*param_1) [32],undefined1 (*param_2) [32],undefined1 (*param_3) [16],
                  ulonglong param_4)

{
  undefined1 auVar1 [16];
  int iVar2;
  ulonglong uVar3;
  char *pcVar4;
  undefined1 (*pauVar5) [32];
  longlong lVar6;
  undefined1 uVar7;
  bool bVar8;
  undefined1 auVar9 [16];
  undefined1 auStack_78 [32];
  undefined1 local_58 [16];
  ulonglong local_48;
  
  local_48 = DAT_14000e040 ^ (ulonglong)auStack_78;
  if (param_4 != 0) {
    if (param_4 == 1) {
      FUN_1400071a0(param_1,param_2,(*param_3)[0]);
    }
    else {
      uVar3 = (longlong)param_2 - (longlong)param_1;
      if (param_4 <= uVar3) {
        if ((((byte)DAT_14000e014 & 4) == 0) || (uVar3 < 0x10)) {
          pauVar5 = (undefined1 (*) [32])((longlong)param_1 + uVar3 + (1 - param_4));
          if (param_1 != pauVar5) {
            lVar6 = (longlong)param_3 - (longlong)param_1;
            do {
              if ((*param_1)[0] == (*param_3)[0]) {
                pcVar4 = *param_1 + 1;
                while (*pcVar4 == pcVar4[lVar6]) {
                  pcVar4 = pcVar4 + 1;
                  if ((longlong)pcVar4 - (longlong)param_1 == param_4) goto LAB_140007423;
                }
              }
              param_1 = (undefined1 (*) [32])(*param_1 + 1);
              lVar6 = lVar6 + -1;
            } while (param_1 != pauVar5);
          }
        }
        else if (param_4 < 0x11) {
          uVar7 = 0x10 < (uint)param_4;
          memcpy(local_58,param_3,param_4);
          auVar1 = local_58;
          do {
            iVar2 = pcmpestri(local_58,*(undefined1 (*) [16])*param_1,0xc);
            if ((bool)uVar7) {
              param_1 = (undefined1 (*) [32])(*param_1 + iVar2);
              if (iVar2 <= (int)(0x10 - (uint)param_4)) goto LAB_140007423;
            }
            else {
              param_1 = (undefined1 (*) [32])(*param_1 + 0x10);
            }
            uVar7 = param_1 < (undefined1 (*) [32])(param_2[-1] + 0x10);
          } while (param_1 <= (undefined1 (*) [32])(param_2[-1] + 0x10));
          if ((longlong)param_2 - (longlong)param_1 != 0) {
            memcpy(local_58,param_1,(longlong)param_2 - (longlong)param_1);
            pcmpestri(auVar1,local_58,0xc);
          }
        }
        else {
          auVar1 = *param_3;
          bVar8 = CARRY8((longlong)param_1 - param_4,uVar3);
          pauVar5 = (undefined1 (*) [32])(((longlong)param_1 - param_4) + uVar3);
          do {
            iVar2 = pcmpestri(auVar1,*(undefined1 (*) [16])*param_1,0xc);
            if (bVar8) {
              if (iVar2 == 0) {
LAB_1400073f5:
                iVar2 = memcmp(*param_1 + 0x10,param_3 + 1,param_4 - 0x10);
                if (iVar2 == 0) break;
              }
              else {
                param_1 = (undefined1 (*) [32])(*param_1 + iVar2);
                if (pauVar5 < param_1) break;
                auVar9._0_4_ = *(uint *)*param_1 ^ auVar1._0_4_;
                auVar9._4_4_ = *(uint *)(*param_1 + 4) ^ auVar1._4_4_;
                auVar9._8_4_ = *(uint *)(*param_1 + 8) ^ auVar1._8_4_;
                auVar9._12_4_ = *(uint *)(*param_1 + 0xc) ^ auVar1._12_4_;
                if (auVar9 == (undefined1  [16])0x0) goto LAB_1400073f5;
              }
              param_1 = (undefined1 (*) [32])(*param_1 + 1);
            }
            else {
              param_1 = (undefined1 (*) [32])(*param_1 + 0x10);
            }
            bVar8 = param_1 < pauVar5;
          } while (param_1 <= pauVar5);
        }
      }
    }
  }
LAB_140007423:
  FUN_140007700(local_48 ^ (ulonglong)auStack_78);
  return;
}



// Function: FUN_140007490 @ 140007490
// Signature: ulonglong FUN_140007490(undefined8 *param_1,uint param_2);


ulonglong FUN_140007490(undefined8 *param_1,uint param_2)

{
  longlong *plVar1;
  uint *puVar2;
  uint uVar3;
  byte *pbVar4;
  int iVar5;
  uint *in_RAX;
  undefined4 extraout_var;
  uint *puVar6;
  undefined1 auVar7 [16];
  uint local_res10 [2];
  
  local_res10[0] = param_2;
  if (param_2 != 0) {
    plVar1 = (longlong *)*param_1;
    puVar2 = (uint *)*plVar1;
    do {
      uVar3 = 0x1f;
      if (local_res10[0] != 0) {
        for (; local_res10[0] >> uVar3 == 0; uVar3 = uVar3 - 1) {
        }
      }
      puVar6 = puVar2;
      if ((uVar3 == 0) ||
         (puVar6 = (uint *)((longlong)puVar2 + (ulonglong)uVar3),
         auVar7._0_4_ = *puVar6 ^ *(uint *)(param_1 + 2),
         auVar7._4_4_ = puVar6[1] ^ *(uint *)((longlong)param_1 + 0x14),
         auVar7._8_4_ = puVar6[2] ^ *(uint *)(param_1 + 3),
         auVar7._12_4_ = puVar6[3] ^ *(uint *)((longlong)param_1 + 0x1c),
         auVar7 == (undefined1  [16])0x0)) {
        iVar5 = memcmp(puVar6 + 4,(void *)param_1[4],param_1[5] - 0x10);
        if (iVar5 == 0) {
          *plVar1 = (longlong)puVar6;
          return CONCAT71((int7)(CONCAT44(extraout_var,iVar5) >> 8),1);
        }
      }
      in_RAX = local_res10;
      pbVar4 = (byte *)((longlong)in_RAX + ((longlong)(int)uVar3 >> 3));
      *pbVar4 = *pbVar4 & ~('\x01' << (uVar3 & 7));
    } while (local_res10[0] != 0);
  }
  return (ulonglong)in_RAX & 0xffffffffffffff00;
}



// Function: thunk_FUN_140006b40 @ 140007520
// Signature: void thunk_FUN_140006b40(undefined1 (*param_1) [32],undefined1 (*param_2) [32],undefined1 (*param_3) [16],ulonglong param_4);


void thunk_FUN_140006b40(undefined1 (*param_1) [32],undefined1 (*param_2) [32],
                        undefined1 (*param_3) [16],ulonglong param_4)

{
  uint uVar1;
  byte *pbVar2;
  uint uVar3;
  int iVar4;
  char *pcVar5;
  uint *puVar6;
  undefined1 (*pauVar7) [32];
  undefined1 (*pauVar8) [32];
  undefined1 (*pauVar9) [16];
  uint uVar10;
  longlong lVar11;
  uint uVar12;
  ulonglong uVar13;
  uint uVar14;
  undefined1 auVar15 [16];
  undefined1 auVar16 [16];
  undefined1 auVar17 [16];
  undefined1 auVar18 [16];
  uint uVar20;
  uint uVar21;
  undefined1 auVar19 [16];
  undefined1 auStack_d8 [32];
  uint auStack_b8 [2];
  undefined1 (*pauStack_b0) [32];
  undefined1 (**appauStack_a8 [2]) [32];
  undefined1 auStack_98 [16];
  undefined1 (*pauStack_88) [16];
  ulonglong uStack_80;
  undefined1 (*pauStack_78) [32];
  undefined8 uStack_70;
  ulonglong uStack_68;
  
  uStack_68 = DAT_14000e040 ^ (ulonglong)auStack_d8;
  pauStack_b0 = param_2;
  if (param_4 != 0) {
    if (param_4 == 1) {
      FUN_140006ef0(param_1,param_2,(*param_3)[0]);
    }
    else {
      uVar13 = (longlong)param_2 - (longlong)param_1;
      if (param_4 <= uVar13) {
        if ((((byte)DAT_14000e014 & 4) == 0) || (uVar13 < 0x10)) {
          pauVar8 = (undefined1 (*) [32])((longlong)param_2 - param_4);
          lVar11 = (longlong)param_3 - (longlong)pauVar8;
          while( true ) {
            if ((*pauVar8)[0] == (*param_3)[0]) {
              pcVar5 = *pauVar8 + 1;
              while (*pcVar5 == pcVar5[lVar11]) {
                pcVar5 = pcVar5 + 1;
                if ((longlong)pcVar5 - (longlong)pauVar8 == param_4) goto LAB_140006ec6;
              }
            }
            if (pauVar8 == param_1) break;
            pauVar8 = (undefined1 (*) [32])(pauVar8[-1] + 0x1f);
            lVar11 = lVar11 + 1;
          }
        }
        else if (param_4 < 0x11) {
          uVar12 = (uint)uVar13 & 0xf;
          uVar14 = (1 << (0x11U - (char)param_4 & 0x1f)) - 1;
          memcpy(&pauStack_78,param_3,param_4);
          auVar17._8_8_ = uStack_70;
          auVar17._0_8_ = pauStack_78;
          pauVar9 = (undefined1 (*) [16])(pauStack_b0[-1] + 0x10);
          auVar15 = pcmpestrm(auVar17,*pauVar9,0xc);
          uVar3 = auVar15._0_4_ & uVar14;
          if (uVar3 == 0) {
            uVar3 = (uint)pauStack_78;
            uVar20 = (uint)((ulonglong)pauStack_78 >> 0x20);
            uVar21 = (uint)((ulonglong)uStack_70 >> 0x20);
            if (pauVar9 != (undefined1 (*) [16])(*param_1 + uVar12)) {
              do {
                pauVar9 = pauVar9 + -1;
                auVar15 = pcmpestrm(auVar17,*pauVar9,0xc);
                uVar10 = auVar15._0_4_;
                if (uVar10 != 0) {
                  auStack_b8[0] = (uVar14 ^ 0xffff) & uVar10;
                  if (auStack_b8[0] != 0) {
                    auVar15 = *(undefined1 (*) [16])(&DAT_14000aa80 + -param_4);
                    do {
                      uVar1 = 0x1f;
                      if (auStack_b8[0] != 0) {
                        for (; auStack_b8[0] >> uVar1 == 0; uVar1 = uVar1 - 1) {
                        }
                      }
                      puVar6 = (uint *)(*pauVar9 + uVar1);
                      auVar16._0_4_ = *puVar6 ^ uVar3;
                      auVar16._4_4_ = puVar6[1] ^ uVar20;
                      auVar16._8_4_ = puVar6[2] ^ (uint)uStack_70;
                      auVar16._12_4_ = puVar6[3] ^ uVar21;
                      if ((auVar15 & auVar16) == (undefined1  [16])0x0) goto LAB_140006ec6;
                      pbVar2 = (byte *)((longlong)auStack_b8 + ((longlong)(int)uVar1 >> 3));
                      *pbVar2 = *pbVar2 & ~('\x01' << (uVar1 & 7));
                    } while (auStack_b8[0] != 0);
                  }
                  uVar10 = uVar10 & uVar14;
                  if (uVar10 != 0) {
                    iVar4 = 0x1f;
                    if (uVar10 != 0) {
                      for (; uVar10 >> iVar4 == 0; iVar4 = iVar4 + -1) {
                      }
                    }
                    goto LAB_140006ec6;
                  }
                }
              } while (pauVar9 != (undefined1 (*) [16])(*param_1 + uVar12));
            }
            if ((ulonglong)uVar12 != 0) {
              auVar17 = pcmpestrm(auVar17,*(undefined1 (*) [16])*param_1,0xc);
              uVar12 = auVar17._0_4_ & (1 << (sbyte)uVar12) - 1U;
              if (uVar12 != 0) {
                auStack_b8[0] = (uVar14 ^ 0xffff) & uVar12;
                if (auStack_b8[0] != 0) {
                  auVar17 = *(undefined1 (*) [16])(&DAT_14000aa80 + -param_4);
                  do {
                    uVar10 = 0x1f;
                    if (auStack_b8[0] != 0) {
                      for (; auStack_b8[0] >> uVar10 == 0; uVar10 = uVar10 - 1) {
                      }
                    }
                    puVar6 = (uint *)(*param_1 + uVar10);
                    auVar15._0_4_ = *puVar6 ^ uVar3;
                    auVar15._4_4_ = puVar6[1] ^ uVar20;
                    auVar15._8_4_ = puVar6[2] ^ (uint)uStack_70;
                    auVar15._12_4_ = puVar6[3] ^ uVar21;
                    if ((auVar17 & auVar15) == (undefined1  [16])0x0) goto LAB_140006ec6;
                    pbVar2 = (byte *)((longlong)auStack_b8 + ((longlong)(int)uVar10 >> 3));
                    *pbVar2 = *pbVar2 & ~('\x01' << (uVar10 & 7));
                  } while (auStack_b8[0] != 0);
                }
                uVar12 = uVar12 & uVar14;
                if ((uVar12 != 0) && (iVar4 = 0x1f, uVar12 != 0)) {
                  for (; uVar12 >> iVar4 == 0; iVar4 = iVar4 + -1) {
                  }
                }
              }
            }
          }
          else {
            iVar4 = 0x1f;
            if (uVar3 != 0) {
              for (; uVar3 >> iVar4 == 0; iVar4 = iVar4 + -1) {
              }
            }
          }
        }
        else {
          auVar17 = *param_3;
          uVar3 = (uint)uVar13 - (int)param_4;
          uStack_80 = param_4;
          pauVar9 = param_3 + 1;
          auStack_98 = auVar17;
          pauStack_88 = pauVar9;
          pauVar8 = (undefined1 (*) [32])((longlong)param_2 - param_4);
          appauStack_a8[0] = &pauStack_78;
          auVar18._0_4_ = *(uint *)*pauVar8 ^ auVar17._0_4_;
          auVar18._4_4_ = *(uint *)(*pauVar8 + 4) ^ auVar17._4_4_;
          auVar18._8_4_ = *(uint *)(*pauVar8 + 8) ^ auVar17._8_4_;
          auVar18._12_4_ = *(uint *)(*pauVar8 + 0xc) ^ auVar17._12_4_;
          pauStack_78 = pauVar8;
          if ((auVar18 != (undefined1  [16])0x0) ||
             (iVar4 = memcmp(*pauVar8 + 0x10,pauVar9,param_4 - 0x10), iVar4 != 0)) {
            while (pauVar8 != (undefined1 (*) [32])(*param_1 + (uVar3 & 0xf))) {
              pauVar8 = (undefined1 (*) [32])(pauVar8[-1] + 0x10);
              pauStack_78 = pauVar8;
              auVar15 = pcmpestrm(auVar17,*(undefined1 (*) [16])*pauVar8,0xc);
              if (auVar15._0_4_ != 0) {
                auStack_b8[0] = auVar15._0_4_;
                do {
                  uVar12 = 0x1f;
                  if (auStack_b8[0] != 0) {
                    for (; auStack_b8[0] >> uVar12 == 0; uVar12 = uVar12 - 1) {
                    }
                  }
                  pauVar7 = pauVar8;
                  if (((uVar12 == 0) ||
                      (pauVar7 = (undefined1 (*) [32])(*pauVar8 + uVar12),
                      auVar19._0_4_ = auVar17._0_4_ ^ *(uint *)*pauVar7,
                      auVar19._4_4_ = auVar17._4_4_ ^ *(uint *)(*pauVar7 + 4),
                      auVar19._8_4_ = auVar17._8_4_ ^ *(uint *)(*pauVar7 + 8),
                      auVar19._12_4_ = auVar17._12_4_ ^ *(uint *)(*pauVar7 + 0xc),
                      auVar19 == (undefined1  [16])0x0)) &&
                     (iVar4 = memcmp(*pauVar7 + 0x10,pauVar9,param_4 - 0x10), iVar4 == 0))
                  goto LAB_140006ec6;
                  pbVar2 = (byte *)((longlong)auStack_b8 + ((longlong)(int)uVar12 >> 3));
                  *pbVar2 = *pbVar2 & ~('\x01' << (uVar12 & 7));
                } while (auStack_b8[0] != 0);
              }
            }
            uVar3 = uVar3 & 0xf;
            if (uVar3 != 0) {
              auVar17 = pcmpestrm(auVar17,*(undefined1 (*) [16])*param_1,0xc);
              uVar3 = auVar17._0_4_ & (1 << (sbyte)uVar3) - 1U;
              pauStack_78 = param_1;
              if (uVar3 != 0) {
                FUN_140007490(appauStack_a8,uVar3);
              }
            }
          }
        }
      }
    }
  }
LAB_140006ec6:
  FUN_140007700(uStack_68 ^ (ulonglong)auStack_d8);
  return;
}



// Function: thunk_FUN_140007040 @ 140007530
// Signature: undefined1 (*) [32]thunk_FUN_140007040(undefined1 (*param_1) [32],undefined1 (*param_2) [32],short param_3);


undefined1 (*) [32]
thunk_FUN_140007040(undefined1 (*param_1) [32],undefined1 (*param_2) [32],short param_3)

{
  ushort uVar1;
  undefined1 auVar2 [32];
  uint uVar3;
  undefined1 (*pauVar4) [32];
  undefined1 (*pauVar5) [32];
  undefined1 (*pauVar6) [32];
  undefined1 (*pauVar7) [32];
  undefined1 (*pauVar8) [32];
  undefined1 (*pauVar9) [32];
  undefined1 (*pauVar10) [32];
  undefined1 (*pauVar11) [32];
  undefined1 (*pauVar12) [32];
  ulonglong uVar13;
  ulonglong uVar14;
  undefined1 auVar15 [16];
  undefined1 auVar16 [32];
  
  uVar13 = (longlong)param_2 - (longlong)param_1;
  pauVar12 = param_2;
  if (((uVar13 & 0xffffffffffffffe0) == 0) || ((DAT_14000e014 & 0x20) == 0)) {
    if (((uVar13 & 0xfffffffffffffff0) != 0) && ((DAT_14000e014 & 4) != 0)) {
      do {
        pauVar4 = pauVar12 + -1;
        pauVar5 = pauVar12 + -1;
        pauVar6 = pauVar12 + -1;
        pauVar7 = pauVar12 + -1;
        pauVar8 = pauVar12 + -1;
        pauVar9 = pauVar12 + -1;
        pauVar10 = pauVar12 + -1;
        pauVar11 = pauVar12 + -1;
        pauVar12 = (undefined1 (*) [32])(pauVar12[-1] + 0x10);
        auVar15._0_2_ = -(ushort)(*(short *)(*pauVar4 + 0x10) == param_3);
        auVar15._2_2_ = -(ushort)(*(short *)(*pauVar5 + 0x12) == param_3);
        auVar15._4_2_ = -(ushort)(*(short *)(*pauVar6 + 0x14) == param_3);
        auVar15._6_2_ = -(ushort)(*(short *)(*pauVar7 + 0x16) == param_3);
        auVar15._8_2_ = -(ushort)(*(short *)(*pauVar8 + 0x18) == param_3);
        auVar15._10_2_ = -(ushort)(*(short *)(*pauVar9 + 0x1a) == param_3);
        auVar15._12_2_ = -(ushort)(*(short *)(*pauVar10 + 0x1c) == param_3);
        auVar15._14_2_ = -(ushort)(*(short *)(*pauVar11 + 0x1e) == param_3);
        uVar1 = (ushort)(SUB161(auVar15 >> 7,0) & 1) | (ushort)(SUB161(auVar15 >> 0xf,0) & 1) << 1 |
                (ushort)(SUB161(auVar15 >> 0x17,0) & 1) << 2 |
                (ushort)(SUB161(auVar15 >> 0x1f,0) & 1) << 3 |
                (ushort)(SUB161(auVar15 >> 0x27,0) & 1) << 4 |
                (ushort)(SUB161(auVar15 >> 0x2f,0) & 1) << 5 |
                (ushort)(SUB161(auVar15 >> 0x37,0) & 1) << 6 |
                (ushort)(SUB161(auVar15 >> 0x3f,0) & 1) << 7 |
                (ushort)(SUB161(auVar15 >> 0x47,0) & 1) << 8 |
                (ushort)(SUB161(auVar15 >> 0x4f,0) & 1) << 9 |
                (ushort)(SUB161(auVar15 >> 0x57,0) & 1) << 10 |
                (ushort)(SUB161(auVar15 >> 0x5f,0) & 1) << 0xb |
                (ushort)(SUB161(auVar15 >> 0x67,0) & 1) << 0xc |
                (ushort)(SUB161(auVar15 >> 0x6f,0) & 1) << 0xd |
                (ushort)((byte)(auVar15._14_2_ >> 7) & 1) << 0xe | auVar15._14_2_ & 0x8000;
        if (uVar1 != 0) {
          uVar3 = 0x1f;
          if (uVar1 != 0) {
            for (; uVar1 >> uVar3 == 0; uVar3 = uVar3 - 1) {
            }
          }
          return (undefined1 (*) [32])(pauVar12[-1] + (ulonglong)uVar3 + 0x1f);
        }
      } while (pauVar12 != (undefined1 (*) [32])((longlong)param_2 - (uVar13 & 0xfffffffffffffff0)))
      ;
    }
  }
  else {
    auVar15 = vpunpcklwd_avx(ZEXT416((uint)(int)param_3),ZEXT416((uint)(int)param_3));
    auVar15 = vpshufd_avx(auVar15,0);
    auVar16._16_16_ = auVar15;
    auVar16._0_16_ = auVar15;
    do {
      auVar2 = vpcmpeqw_avx2(auVar16,pauVar12[-1]);
      pauVar12 = pauVar12 + -1;
      uVar3 = (uint)(SUB321(auVar2 >> 7,0) & 1) | (uint)(SUB321(auVar2 >> 0xf,0) & 1) << 1 |
              (uint)(SUB321(auVar2 >> 0x17,0) & 1) << 2 | (uint)(SUB321(auVar2 >> 0x1f,0) & 1) << 3
              | (uint)(SUB321(auVar2 >> 0x27,0) & 1) << 4 |
              (uint)(SUB321(auVar2 >> 0x2f,0) & 1) << 5 | (uint)(SUB321(auVar2 >> 0x37,0) & 1) << 6
              | (uint)(SUB321(auVar2 >> 0x3f,0) & 1) << 7 |
              (uint)(SUB321(auVar2 >> 0x47,0) & 1) << 8 | (uint)(SUB321(auVar2 >> 0x4f,0) & 1) << 9
              | (uint)(SUB321(auVar2 >> 0x57,0) & 1) << 10 |
              (uint)(SUB321(auVar2 >> 0x5f,0) & 1) << 0xb |
              (uint)(SUB321(auVar2 >> 0x67,0) & 1) << 0xc |
              (uint)(SUB321(auVar2 >> 0x6f,0) & 1) << 0xd |
              (uint)(SUB321(auVar2 >> 0x77,0) & 1) << 0xe | (uint)SUB321(auVar2 >> 0x7f,0) << 0xf |
              (uint)(SUB321(auVar2 >> 0x87,0) & 1) << 0x10 |
              (uint)(SUB321(auVar2 >> 0x8f,0) & 1) << 0x11 |
              (uint)(SUB321(auVar2 >> 0x97,0) & 1) << 0x12 |
              (uint)(SUB321(auVar2 >> 0x9f,0) & 1) << 0x13 |
              (uint)(SUB321(auVar2 >> 0xa7,0) & 1) << 0x14 |
              (uint)(SUB321(auVar2 >> 0xaf,0) & 1) << 0x15 |
              (uint)(SUB321(auVar2 >> 0xb7,0) & 1) << 0x16 | (uint)SUB321(auVar2 >> 0xbf,0) << 0x17
              | (uint)(SUB321(auVar2 >> 199,0) & 1) << 0x18 |
              (uint)(SUB321(auVar2 >> 0xcf,0) & 1) << 0x19 |
              (uint)(SUB321(auVar2 >> 0xd7,0) & 1) << 0x1a |
              (uint)(SUB321(auVar2 >> 0xdf,0) & 1) << 0x1b |
              (uint)(SUB321(auVar2 >> 0xe7,0) & 1) << 0x1c |
              (uint)(SUB321(auVar2 >> 0xef,0) & 1) << 0x1d |
              (uint)(SUB321(auVar2 >> 0xf7,0) & 1) << 0x1e | (uint)(byte)(auVar2[0x1f] >> 7) << 0x1f
      ;
      if (uVar3 != 0) {
        return (undefined1 (*) [32])(pauVar12[-1] + (ulonglong)(0x1f - LZCOUNT(uVar3)) + 0x1f);
      }
    } while (pauVar12 != (undefined1 (*) [32])((longlong)param_2 - (uVar13 & 0xffffffffffffffe0)));
    uVar14 = (ulonglong)((uint)uVar13 & 0x1c);
    if ((uVar13 & 0x1c) != 0) {
      pauVar12 = (undefined1 (*) [32])((longlong)pauVar12 - uVar14);
      auVar2 = vpmaskmovd_avx2(*(undefined1 (*) [32])(&DAT_14000aa50 + -uVar14),*pauVar12);
      auVar16 = vpcmpeqw_avx2(auVar2,auVar16);
      auVar16 = vpand_avx2(auVar16,*(undefined1 (*) [32])(&DAT_14000aa50 + -uVar14));
      uVar3 = (uint)(SUB321(auVar16 >> 7,0) & 1) | (uint)(SUB321(auVar16 >> 0xf,0) & 1) << 1 |
              (uint)(SUB321(auVar16 >> 0x17,0) & 1) << 2 |
              (uint)(SUB321(auVar16 >> 0x1f,0) & 1) << 3 |
              (uint)(SUB321(auVar16 >> 0x27,0) & 1) << 4 |
              (uint)(SUB321(auVar16 >> 0x2f,0) & 1) << 5 |
              (uint)(SUB321(auVar16 >> 0x37,0) & 1) << 6 |
              (uint)(SUB321(auVar16 >> 0x3f,0) & 1) << 7 |
              (uint)(SUB321(auVar16 >> 0x47,0) & 1) << 8 |
              (uint)(SUB321(auVar16 >> 0x4f,0) & 1) << 9 |
              (uint)(SUB321(auVar16 >> 0x57,0) & 1) << 10 |
              (uint)(SUB321(auVar16 >> 0x5f,0) & 1) << 0xb |
              (uint)(SUB321(auVar16 >> 0x67,0) & 1) << 0xc |
              (uint)(SUB321(auVar16 >> 0x6f,0) & 1) << 0xd |
              (uint)(SUB321(auVar16 >> 0x77,0) & 1) << 0xe | (uint)SUB321(auVar16 >> 0x7f,0) << 0xf
              | (uint)(SUB321(auVar16 >> 0x87,0) & 1) << 0x10 |
              (uint)(SUB321(auVar16 >> 0x8f,0) & 1) << 0x11 |
              (uint)(SUB321(auVar16 >> 0x97,0) & 1) << 0x12 |
              (uint)(SUB321(auVar16 >> 0x9f,0) & 1) << 0x13 |
              (uint)(SUB321(auVar16 >> 0xa7,0) & 1) << 0x14 |
              (uint)(SUB321(auVar16 >> 0xaf,0) & 1) << 0x15 |
              (uint)(SUB321(auVar16 >> 0xb7,0) & 1) << 0x16 |
              (uint)SUB321(auVar16 >> 0xbf,0) << 0x17 | (uint)(SUB321(auVar16 >> 199,0) & 1) << 0x18
              | (uint)(SUB321(auVar16 >> 0xcf,0) & 1) << 0x19 |
              (uint)(SUB321(auVar16 >> 0xd7,0) & 1) << 0x1a |
              (uint)(SUB321(auVar16 >> 0xdf,0) & 1) << 0x1b |
              (uint)(SUB321(auVar16 >> 0xe7,0) & 1) << 0x1c |
              (uint)(SUB321(auVar16 >> 0xef,0) & 1) << 0x1d |
              (uint)(SUB321(auVar16 >> 0xf7,0) & 1) << 0x1e |
              (uint)(byte)(auVar16[0x1f] >> 7) << 0x1f;
      if (uVar3 != 0) {
        return (undefined1 (*) [32])(pauVar12[-1] + (ulonglong)(0x1f - LZCOUNT(uVar3)) + 0x1f);
      }
    }
  }
  do {
    if (pauVar12 == param_1) {
      return param_2;
    }
    pauVar12 = (undefined1 (*) [32])(pauVar12[-1] + 0x1e);
  } while (*(short *)*pauVar12 != param_3);
  return pauVar12;
}



// Function: thunk_FUN_1400072a0 @ 140007540
// Signature: void thunk_FUN_1400072a0(undefined1 (*param_1) [32],undefined1 (*param_2) [32],undefined1 (*param_3) [16],ulonglong param_4);


void thunk_FUN_1400072a0(undefined1 (*param_1) [32],undefined1 (*param_2) [32],
                        undefined1 (*param_3) [16],ulonglong param_4)

{
  undefined1 auVar1 [16];
  int iVar2;
  ulonglong uVar3;
  char *pcVar4;
  undefined1 (*pauVar5) [32];
  longlong lVar6;
  undefined1 uVar7;
  bool bVar8;
  undefined1 auVar9 [16];
  undefined1 auStack_78 [32];
  undefined1 auStack_58 [16];
  ulonglong uStack_48;
  
  uStack_48 = DAT_14000e040 ^ (ulonglong)auStack_78;
  if (param_4 != 0) {
    if (param_4 == 1) {
      FUN_1400071a0(param_1,param_2,(*param_3)[0]);
    }
    else {
      uVar3 = (longlong)param_2 - (longlong)param_1;
      if (param_4 <= uVar3) {
        if ((((byte)DAT_14000e014 & 4) == 0) || (uVar3 < 0x10)) {
          pauVar5 = (undefined1 (*) [32])((longlong)param_1 + uVar3 + (1 - param_4));
          if (param_1 != pauVar5) {
            lVar6 = (longlong)param_3 - (longlong)param_1;
            do {
              if ((*param_1)[0] == (*param_3)[0]) {
                pcVar4 = *param_1 + 1;
                while (*pcVar4 == pcVar4[lVar6]) {
                  pcVar4 = pcVar4 + 1;
                  if ((longlong)pcVar4 - (longlong)param_1 == param_4) goto LAB_140007423;
                }
              }
              param_1 = (undefined1 (*) [32])(*param_1 + 1);
              lVar6 = lVar6 + -1;
            } while (param_1 != pauVar5);
          }
        }
        else if (param_4 < 0x11) {
          uVar7 = 0x10 < (uint)param_4;
          memcpy(auStack_58,param_3,param_4);
          auVar1 = auStack_58;
          do {
            iVar2 = pcmpestri(auStack_58,*(undefined1 (*) [16])*param_1,0xc);
            if ((bool)uVar7) {
              param_1 = (undefined1 (*) [32])(*param_1 + iVar2);
              if (iVar2 <= (int)(0x10 - (uint)param_4)) goto LAB_140007423;
            }
            else {
              param_1 = (undefined1 (*) [32])(*param_1 + 0x10);
            }
            uVar7 = param_1 < (undefined1 (*) [32])(param_2[-1] + 0x10);
          } while (param_1 <= (undefined1 (*) [32])(param_2[-1] + 0x10));
          if ((longlong)param_2 - (longlong)param_1 != 0) {
            memcpy(auStack_58,param_1,(longlong)param_2 - (longlong)param_1);
            pcmpestri(auVar1,auStack_58,0xc);
          }
        }
        else {
          auVar1 = *param_3;
          bVar8 = CARRY8((longlong)param_1 - param_4,uVar3);
          pauVar5 = (undefined1 (*) [32])(((longlong)param_1 - param_4) + uVar3);
          do {
            iVar2 = pcmpestri(auVar1,*(undefined1 (*) [16])*param_1,0xc);
            if (bVar8) {
              if (iVar2 == 0) {
LAB_1400073f5:
                iVar2 = memcmp(*param_1 + 0x10,param_3 + 1,param_4 - 0x10);
                if (iVar2 == 0) break;
              }
              else {
                param_1 = (undefined1 (*) [32])(*param_1 + iVar2);
                if (pauVar5 < param_1) break;
                auVar9._0_4_ = *(uint *)*param_1 ^ auVar1._0_4_;
                auVar9._4_4_ = *(uint *)(*param_1 + 4) ^ auVar1._4_4_;
                auVar9._8_4_ = *(uint *)(*param_1 + 8) ^ auVar1._8_4_;
                auVar9._12_4_ = *(uint *)(*param_1 + 0xc) ^ auVar1._12_4_;
                if (auVar9 == (undefined1  [16])0x0) goto LAB_1400073f5;
              }
              param_1 = (undefined1 (*) [32])(*param_1 + 1);
            }
            else {
              param_1 = (undefined1 (*) [32])(*param_1 + 0x10);
            }
            bVar8 = param_1 < pauVar5;
          } while (param_1 <= pauVar5);
        }
      }
    }
  }
LAB_140007423:
  FUN_140007700(uStack_48 ^ (ulonglong)auStack_78);
  return;
}



// Function: _Xbad_alloc @ 14000754b
// Signature: void __cdecl std::_Xbad_alloc(void);


void __cdecl std::_Xbad_alloc(void)

{
                    /* WARNING: Could not recover jumptable at 0x00014000754b. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  _Xbad_alloc();
  return;
}



// Function: FUN_140007554 @ 140007554
// Signature: void FUN_140007554(undefined8 param_1);


void FUN_140007554(undefined8 param_1)

{
  code *pcVar1;
  undefined8 *puVar2;
  
  puVar2 = (undefined8 *)malloc(0x10);
  if (puVar2 != (undefined8 *)0x0) {
    *puVar2 = DAT_14000e3b0;
    puVar2[1] = param_1;
    DAT_14000e3b0 = puVar2;
    return;
  }
  std::_Xbad_alloc();
  pcVar1 = (code *)swi(3);
  (*pcVar1)();
  return;
}



// Function: Sleep @ 140007594
// Signature: void __stdcall Sleep(DWORD dwMilliseconds);


void __stdcall Sleep(DWORD dwMilliseconds)

{
                    /* WARNING: Could not recover jumptable at 0x000140007594. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  Sleep(dwMilliseconds);
  return;
}



// Function: showmanyc @ 14000759b
// Signature: __int64 __thiscall std::basic_streambuf<char,struct_std::char_traits<char>_>::showmanyc(basic_streambuf<char,struct_std::char_traits<char>_> *this);


__int64 __thiscall
std::basic_streambuf<char,struct_std::char_traits<char>_>::showmanyc
          (basic_streambuf<char,struct_std::char_traits<char>_> *this)

{
  __int64 _Var1;
  
                    /* WARNING: Could not recover jumptable at 0x00014000759b. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  _Var1 = showmanyc(this);
  return _Var1;
}



// Function: _CxxThrowException @ 1400075b3
// Signature: void __stdcall _CxxThrowException(void *pExceptionObject,ThrowInfo *pThrowInfo);


void __stdcall _CxxThrowException(void *pExceptionObject,ThrowInfo *pThrowInfo)

{
                    /* WARNING: Could not recover jumptable at 0x0001400075b3. Too many branches */
                    /* WARNING: Subroutine does not return */
                    /* WARNING: Treating indirect jump as call */
  _CxxThrowException(pExceptionObject,pThrowInfo);
  return;
}



// Function: __current_exception @ 1400075b9
// Signature: void __current_exception(void);


void __current_exception(void)

{
                    /* WARNING: Could not recover jumptable at 0x0001400075b9. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  __current_exception();
  return;
}



// Function: __current_exception_context @ 1400075bf
// Signature: void __current_exception_context(void);


void __current_exception_context(void)

{
                    /* WARNING: Could not recover jumptable at 0x0001400075bf. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  __current_exception_context();
  return;
}



// Function: memset @ 1400075c5
// Signature: void * __cdecl memset(void *_Dst,int _Val,size_t _Size);


void * __cdecl memset(void *_Dst,int _Val,size_t _Size)

{
  void *pvVar1;
  
                    /* WARNING: Could not recover jumptable at 0x0001400075c5. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  pvVar1 = memset(_Dst,_Val,_Size);
  return pvVar1;
}



// Function: memcmp @ 1400075cb
// Signature: int __cdecl memcmp(void *_Buf1,void *_Buf2,size_t _Size);


int __cdecl memcmp(void *_Buf1,void *_Buf2,size_t _Size)

{
  int iVar1;
  
                    /* WARNING: Could not recover jumptable at 0x0001400075cb. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  iVar1 = memcmp(_Buf1,_Buf2,_Size);
  return iVar1;
}



// Function: memcpy @ 1400075d1
// Signature: void * __cdecl memcpy(void *_Dst,void *_Src,size_t _Size);


void * __cdecl memcpy(void *_Dst,void *_Src,size_t _Size)

{
  void *pvVar1;
  
                    /* WARNING: Could not recover jumptable at 0x0001400075d1. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  pvVar1 = memcpy(_Dst,_Src,_Size);
  return pvVar1;
}



// Function: _callnewh @ 1400075d7
// Signature: int __cdecl _callnewh(size_t _Size);


int __cdecl _callnewh(size_t _Size)

{
  int iVar1;
  
                    /* WARNING: Could not recover jumptable at 0x0001400075d7. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  iVar1 = _callnewh(_Size);
  return iVar1;
}



// Function: malloc @ 1400075dd
// Signature: void * __cdecl malloc(size_t _Size);


void * __cdecl malloc(size_t _Size)

{
  void *pvVar1;
  
                    /* WARNING: Could not recover jumptable at 0x0001400075dd. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  pvVar1 = malloc(_Size);
  return pvVar1;
}



// Function: free @ 1400075e3
// Signature: void __cdecl free(void *_Memory);


void __cdecl free(void *_Memory)

{
                    /* WARNING: Could not recover jumptable at 0x0001400075e3. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  free(_Memory);
  return;
}



// Function: _seh_filter_exe @ 1400075e9
// Signature: void _seh_filter_exe(void);


void _seh_filter_exe(void)

{
                    /* WARNING: Could not recover jumptable at 0x0001400075e9. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  _seh_filter_exe();
  return;
}



// Function: _set_app_type @ 1400075ef
// Signature: void _set_app_type(void);


void _set_app_type(void)

{
                    /* WARNING: Could not recover jumptable at 0x0001400075ef. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  _set_app_type();
  return;
}



// Function: __setusermatherr @ 1400075f5
// Signature: void __setusermatherr(void);


void __setusermatherr(void)

{
                    /* WARNING: Could not recover jumptable at 0x0001400075f5. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  __setusermatherr();
  return;
}



// Function: _configure_narrow_argv @ 1400075fb
// Signature: void _configure_narrow_argv(void);


void _configure_narrow_argv(void)

{
                    /* WARNING: Could not recover jumptable at 0x0001400075fb. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  _configure_narrow_argv();
  return;
}



// Function: _initialize_narrow_environment @ 140007601
// Signature: void _initialize_narrow_environment(void);


void _initialize_narrow_environment(void)

{
                    /* WARNING: Could not recover jumptable at 0x000140007601. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  _initialize_narrow_environment();
  return;
}



// Function: _get_narrow_winmain_command_line @ 140007607
// Signature: void _get_narrow_winmain_command_line(void);


void _get_narrow_winmain_command_line(void)

{
                    /* WARNING: Could not recover jumptable at 0x000140007607. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  _get_narrow_winmain_command_line();
  return;
}



// Function: _initterm @ 14000760d
// Signature: void _initterm(void);


void _initterm(void)

{
                    /* WARNING: Could not recover jumptable at 0x00014000760d. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  _initterm();
  return;
}



// Function: _initterm_e @ 140007613
// Signature: void _initterm_e(void);


void _initterm_e(void)

{
                    /* WARNING: Could not recover jumptable at 0x000140007613. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  _initterm_e();
  return;
}



// Function: exit @ 140007619
// Signature: void __cdecl exit(int _Code);


void __cdecl exit(int _Code)

{
                    /* WARNING: Could not recover jumptable at 0x000140007619. Too many branches */
                    /* WARNING: Subroutine does not return */
                    /* WARNING: Treating indirect jump as call */
  exit(_Code);
  return;
}



// Function: _exit @ 14000761f
// Signature: void __cdecl _exit(int _Code);


void __cdecl _exit(int _Code)

{
                    /* WARNING: Could not recover jumptable at 0x00014000761f. Too many branches */
                    /* WARNING: Subroutine does not return */
                    /* WARNING: Treating indirect jump as call */
  _exit(_Code);
  return;
}



// Function: _set_fmode @ 140007625
// Signature: errno_t __cdecl _set_fmode(int _Mode);


errno_t __cdecl _set_fmode(int _Mode)

{
  errno_t eVar1;
  
                    /* WARNING: Could not recover jumptable at 0x000140007625. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  eVar1 = _set_fmode(_Mode);
  return eVar1;
}



// Function: _cexit @ 14000762b
// Signature: void __cdecl _cexit(void);


void __cdecl _cexit(void)

{
                    /* WARNING: Could not recover jumptable at 0x00014000762b. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  _cexit();
  return;
}



// Function: _register_thread_local_exe_atexit_callback @ 140007637
// Signature: void _register_thread_local_exe_atexit_callback(void);


void _register_thread_local_exe_atexit_callback(void)

{
                    /* WARNING: Could not recover jumptable at 0x000140007637. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  _register_thread_local_exe_atexit_callback();
  return;
}



// Function: _configthreadlocale @ 14000763d
// Signature: int __cdecl _configthreadlocale(int _Flag);


int __cdecl _configthreadlocale(int _Flag)

{
  int iVar1;
  
                    /* WARNING: Could not recover jumptable at 0x00014000763d. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  iVar1 = _configthreadlocale(_Flag);
  return iVar1;
}



// Function: __p__commode @ 140007649
// Signature: void __p__commode(void);


void __p__commode(void)

{
                    /* WARNING: Could not recover jumptable at 0x000140007649. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  __p__commode();
  return;
}



// Function: _initialize_onexit_table @ 14000764f
// Signature: void _initialize_onexit_table(void);


void _initialize_onexit_table(void)

{
                    /* WARNING: Could not recover jumptable at 0x00014000764f. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  _initialize_onexit_table();
  return;
}



// Function: _register_onexit_function @ 140007655
// Signature: void _register_onexit_function(void);


void _register_onexit_function(void)

{
                    /* WARNING: Could not recover jumptable at 0x000140007655. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  _register_onexit_function();
  return;
}



// Function: _crt_atexit @ 14000765b
// Signature: void _crt_atexit(void);


void _crt_atexit(void)

{
                    /* WARNING: Could not recover jumptable at 0x00014000765b. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  _crt_atexit();
  return;
}



// Function: terminate @ 140007661
// Signature: void terminate(void);


void terminate(void)

{
                    /* WARNING: Could not recover jumptable at 0x000140007661. Too many branches */
                    /* WARNING: Subroutine does not return */
                    /* WARNING: Treating indirect jump as call */
  terminate();
  return;
}



// Function: FUN_140007688 @ 140007688
// Signature: void FUN_140007688(ulonglong param_1,longlong param_2,uint *param_3);


void FUN_140007688(ulonglong param_1,longlong param_2,uint *param_3)

{
  ulonglong uVar1;
  ulonglong uVar2;
  
  uVar2 = param_1;
  if ((*param_3 & 4) != 0) {
    uVar2 = (longlong)(int)param_3[1] + param_1 & (longlong)(int)-param_3[2];
  }
  uVar1 = (ulonglong)*(uint *)(*(longlong *)(param_2 + 0x10) + 8);
  if ((*(byte *)(uVar1 + 3 + *(longlong *)(param_2 + 8)) & 0xf) != 0) {
    param_1 = param_1 + ((ulonglong)*(byte *)(uVar1 + 3 + *(longlong *)(param_2 + 8)) & 0xfffffff0);
  }
  FUN_140007700(param_1 ^ *(ulonglong *)((longlong)(int)(*param_3 & 0xfffffff8) + uVar2));
  return;
}



// Function: FUN_140007700 @ 140007700
// Signature: void FUN_140007700(longlong param_1);


void FUN_140007700(longlong param_1)

{
  if ((param_1 == DAT_14000e040) && ((short)((ulonglong)param_1 >> 0x30) == 0)) {
    return;
  }
  FUN_140007754();
  return;
}



// Function: __raise_securityfailure @ 140007720
// Signature: void __raise_securityfailure(_EXCEPTION_POINTERS *param_1);


/* Library Function - Single Match
    __raise_securityfailure
   
   Libraries: Visual Studio 2015 Release, Visual Studio 2017 Release, Visual Studio 2019 Release */

void __raise_securityfailure(_EXCEPTION_POINTERS *param_1)

{
  HANDLE pvVar1;
  
  SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)0x0);
  UnhandledExceptionFilter(param_1);
  pvVar1 = GetCurrentProcess();
                    /* WARNING: Could not recover jumptable at 0x00014000774d. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  TerminateProcess(pvVar1,0xc0000409);
  return;
}



// Function: FUN_140007754 @ 140007754
// Signature: void FUN_140007754(void);


/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_140007754(void)

{
  code *pcVar1;
  BOOL BVar2;
  undefined1 *puVar3;
  undefined1 auStack_38 [8];
  undefined1 auStack_30 [48];
  
  puVar3 = auStack_38;
  BVar2 = IsProcessorFeaturePresent(0x17);
  if (BVar2 != 0) {
    pcVar1 = (code *)swi(0x29);
    (*pcVar1)(2);
    puVar3 = auStack_30;
  }
  *(undefined8 *)(puVar3 + -8) = 0x14000777f;
  FUN_140007828((PCONTEXT)&DAT_14000e460);
  _DAT_14000e3d0 = *(undefined8 *)(puVar3 + 0x38);
  _DAT_14000e4f8 = puVar3 + 0x40;
  _DAT_14000e4e0 = *(undefined8 *)(puVar3 + 0x40);
  _DAT_14000e3c0 = 0xc0000409;
  _DAT_14000e3c4 = 1;
  _DAT_14000e3d8 = 1;
  DAT_14000e3e0 = 2;
  *(undefined8 *)(puVar3 + 0x20) = DAT_14000e040;
  *(undefined8 *)(puVar3 + 0x28) = DAT_14000e080;
  *(undefined8 *)(puVar3 + -8) = 0x140007821;
  DAT_14000e558 = _DAT_14000e3d0;
  __raise_securityfailure((_EXCEPTION_POINTERS *)&PTR_DAT_14000aa90);
  return;
}



// Function: FUN_140007828 @ 140007828
// Signature: void FUN_140007828(PCONTEXT param_1);


void FUN_140007828(PCONTEXT param_1)

{
  DWORD64 ControlPc;
  PRUNTIME_FUNCTION FunctionEntry;
  int iVar1;
  DWORD64 local_res8;
  ulonglong local_res10;
  PVOID local_res18 [2];
  
  RtlCaptureContext();
  ControlPc = param_1->Rip;
  iVar1 = 0;
  do {
    FunctionEntry = RtlLookupFunctionEntry(ControlPc,&local_res8,(PUNWIND_HISTORY_TABLE)0x0);
    if (FunctionEntry == (PRUNTIME_FUNCTION)0x0) {
      return;
    }
    RtlVirtualUnwind(0,local_res8,ControlPc,FunctionEntry,param_1,local_res18,&local_res10,
                     (PKNONVOLATILE_CONTEXT_POINTERS)0x0);
    iVar1 = iVar1 + 1;
  } while (iVar1 < 2);
  return;
}



// Function: memmove @ 14000789c
// Signature: void * __cdecl memmove(void *_Dst,void *_Src,size_t _Size);


void * __cdecl memmove(void *_Dst,void *_Src,size_t _Size)

{
  void *pvVar1;
  
                    /* WARNING: Could not recover jumptable at 0x00014000789c. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  pvVar1 = memmove(_Dst,_Src,_Size);
  return pvVar1;
}



// Function: _guard_dispatch_icall @ 1400078c0
// Signature: void _guard_dispatch_icall(void);


/* WARNING: This is an inlined function */

void _guard_dispatch_icall(void)

{
  code *UNRECOVERED_JUMPTABLE;
  
                    /* WARNING: Could not recover jumptable at 0x0001400078c0. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  (*UNRECOVERED_JUMPTABLE)();
  return;
}



// Function: FUN_1400079e0 @ 1400079e0
// Signature: undefined8 FUN_1400079e0(undefined8 param_1,longlong param_2);


undefined8 FUN_1400079e0(undefined8 param_1,longlong param_2)

{
  std::basic_ios<char,struct_std::char_traits<char>_>::setstate
            ((basic_ios<char,struct_std::char_traits<char>_> *)
             ((longlong)*(int *)(**(longlong **)(param_2 + 0x70) + 4) +
             (longlong)*(longlong **)(param_2 + 0x70)),4,true);
  return 0;
}



// Function: FUN_140007a40 @ 140007a40
// Signature: void FUN_140007a40(undefined8 param_1,longlong param_2);


void FUN_140007a40(undefined8 param_1,longlong param_2)

{
  if ((*(uint *)(param_2 + 0x78) & 1) != 0) {
    *(uint *)(param_2 + 0x78) = *(uint *)(param_2 + 0x78) & 0xfffffffe;
    std::basic_ios<char,struct_std::char_traits<char>_>::
    ~basic_ios<char,struct_std::char_traits<char>_>
              ((basic_ios<char,struct_std::char_traits<char>_> *)
               (*(longlong *)(param_2 + 0x60) + 0xb0));
  }
  return;
}



// Function: FUN_140007ae4 @ 140007ae4
// Signature: void FUN_140007ae4(void);


void FUN_140007ae4(void)

{
  undefined8 *_Memory;
  longlong lVar1;
  
  while (_Memory = DAT_14000e3b0, DAT_14000e3b0 != (undefined8 *)0x0) {
    DAT_14000e3b0 = (undefined8 *)*DAT_14000e3b0;
    lVar1 = (*(code *)PTR__guard_dispatch_icall_140008420)();
    if (lVar1 != 0) {
      (*(code *)PTR__guard_dispatch_icall_140008420)(lVar1,1);
    }
    free(_Memory);
  }
  return;
}



