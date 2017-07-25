/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla Universal charset detector code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2001
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *          Shy Shalom <shooshX@gmail.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */
#include <stdio.h>
#include <string.h>
#include "nsSBCharSetProber.h"

int thresh = 0;


void nsSingleByteCharSetProber::BitShift(unsigned char* aBuf, unsigned char* copy_aBuf, PRUint32 start, PRUint32 aLen, PRUint32 shift_bit){
  unsigned char LSB,pre_LSB;

  for(PRUint32 i = 0; i < aLen; i++)
    copy_aBuf[i] = (unsigned char)aBuf[i];

  while(shift_bit--){
    for(PRUint32 i = 0; i < aLen; i++){
      LSB = copy_aBuf[i] & 0x01;
      copy_aBuf[i] >>= 1;

      if (i != start && pre_LSB)
        copy_aBuf[i] |= 0x80;
      else
        copy_aBuf[i] &= 0x7F;
      pre_LSB = LSB;
    }
  }
}

PRUint32 nsSingleByteCharSetProber::GetScore(unsigned char shift_bit[][65536], PRUint32 start, PRUint32 aLen){

  PRUint32 Max_Score = 9999;
  PRUint32 Max_score_bit;
  PRUint32 Score[8];
  unsigned char order;
  unsigned char lastorder;
  int check_len;

  memset(Score, 0, sizeof(PRUint32)*8);
  lastorder = mLastOrder;

  for(PRUint32 shift = 0; shift < 8; shift++){              // 이후 0이 발생한 수중 제일 적은 쉬프트를 사용
    check_len = start + 35;
    for(PRUint32 i = start; i < check_len; i++){
      if(i == aLen)
        break;
      order = mModel->charToOrderMap[(unsigned char)shift_bit[shift][i]];
      if(order < mModel->freqCharCount){
        if (mLastOrder < mModel->freqCharCount){
          if(mModel->precedenceMatrix[mLastOrder*mModel->freqCharCount+order] == 0){
            Score[shift]++;
          }
        }
        mLastOrder = order;
      }
    }
    mLastOrder = lastorder;
  }

  for(PRUint32 shift = 0; shift < 8; shift++){      //최소값 구하는거
    if(Score[shift] < Max_Score){
      Max_Score = Score[shift];
      Max_score_bit = shift;
    }
  }

  //printf("Max_Score : %d \n", Max_Score);
  return Max_score_bit;
}

//심볼이 연속 2~3번 나올때 의심 한번 해보기

void nsSingleByteCharSetProber::ErrorRecover(unsigned char* aBuf, PRUint32 aLen){

  unsigned char order;
  unsigned char lastorder;
  PRUint32 Max_score_shift;
  PRUint32 shift_count = 0;

  Reset();
  mLastOrder = mModel->charToOrderMap[(unsigned char)aBuf[0]];

  for(PRUint32 i = 1; i < aLen; i++){
    order = mModel->charToOrderMap[(unsigned char)aBuf[i]];
    lastorder = mLastOrder;
      if(order < mModel->freqCharCount){
        if (mLastOrder < mModel->freqCharCount){
          if(mModel->precedenceMatrix[mLastOrder*mModel->freqCharCount+order] == 0){ //0번 클래스
            //printf("Error Discover : %d \n", i);
            unsigned char shift[8][65536];

            for(PRUint32 j = 0; j < 8; j++){                // 쉬프뜨
              BitShift(aBuf, shift[j], i, aLen, j);
            }
            Max_score_shift = GetScore(shift, i+1, aLen);     //score 계산
            //printf("Shift_bit : %d \n", Max_score_shift);

            for(PRUint32 k = i; k < aLen; k++){        //제일 좋은 스코어의 것으로 buf내용 변경
              aBuf[k] = shift[Max_score_shift][k];
            }
            aBuf[i] = '?';        //ILL이 발생한 비트를 ?로 바꿔준다*/
            mLastOrder = lastorder;
            shift_count++;
          }
          else{
            mLastOrder = order;
          }
        }
        else{
          mLastOrder = order;
        }
      }
      else{
        mLastOrder = order;
      }
  }
}

nsProbingState nsSingleByteCharSetProber::HandleData(const char* aBuf, PRUint32 aLen)
{
  unsigned char order;
  unsigned char copy_aBuf[65536];
  PRUint32 error_count = 0;
  char *newBuf1 = 0;
  PRUint32 newLen1 = 0;

  for(PRUint32 i = 0; i < aLen; i++)
    copy_aBuf[i] = (unsigned char)aBuf[i];

  ErrorRecover(copy_aBuf, aLen);

  char buffer[50];

  sprintf(buffer, "z_%s_recover.txt",GetCharSetName());

  FILE* out = fopen(buffer, "a+");
  for (PRUint32 k = 0; k < aLen+1; k++)
    fputc(copy_aBuf[k], out);

  fclose(out);

  FilterWithoutEnglishLetters((const char*)copy_aBuf, aLen, &newBuf1, newLen1);
  //FilterWithoutEnglishLetters(aBuf, aLen, &newBuf1, newLen1);
  thresh = newLen1/5;
  Reset();

  for (PRUint32 i = 0; i < newLen1; i++)
  {
    order = mModel->charToOrderMap[(unsigned char)newBuf1[i]];
    if (order < SYMBOL_CAT_ORDER)
    {
      mTotalChar++;
    }
    else if (order == ILL)    //ILL발생해도 break 하지 않음
    {
      /* When encountering an illegal codepoint, no need
       * to continue analyzing data. */
       error_count++;       //에러 발생 수
      //mState = eNotMe;
      //break;
    }
    else if (order == CTR)
    {
      mCtrlChar++;
    }
    if (order < mModel->freqCharCount)
    {
        mFreqChar++;

      if (mLastOrder < mModel->freqCharCount)
      {
        mTotalSeqs++;
        if (!mReversed){
          ++(mSeqCounters[mModel->precedenceMatrix[mLastOrder*mModel->freqCharCount+order]]);
        }
        else // reverse the order of the letters in the lookup
          ++(mSeqCounters[mModel->precedenceMatrix[order*mModel->freqCharCount+mLastOrder]]);
      }
    }
    mLastOrder = order;
  }
  printf("%s : 0 : %d, 1 : %d 2 : %d 3 : %d\n", GetCharSetName(), mSeqCounters[0],mSeqCounters[1],mSeqCounters[2],mSeqCounters[3]);
  if (mState == eDetecting)
    if (mTotalSeqs > SB_ENOUGH_REL_THRESHOLD)
    {
      float cf = GetConfidence();
      if (cf > POSITIVE_SHORTCUT_THRESHOLD){
        mState = eFoundIt;
      }
      else if (cf < NEGATIVE_SHORTCUT_THRESHOLD){
        mState = eNotMe;
      }
    }
  return mState;
}

void  nsSingleByteCharSetProber::Reset(void)
{
  mState = eDetecting;
  mLastOrder = 255;
  for (PRUint32 i = 0; i < NUMBER_OF_SEQ_CAT; i++)
    mSeqCounters[i] = 0;
  mTotalSeqs = 0;
  mTotalChar = 0;
  mCtrlChar  = 0;
  mFreqChar = 0;
}

//#define NEGATIVE_APPROACH 1

float nsSingleByteCharSetProber::GetConfidence(void)
{
#ifdef NEGATIVE_APPROACH
  if (mTotalSeqs > 0)
    if (mTotalSeqs > mSeqCounters[NEGATIVE_CAT]*10 )
      return ((float)(mTotalSeqs - mSeqCounters[NEGATIVE_CAT]*10))/mTotalSeqs * mFreqChar / mTotalChar;
  return (float)0.01;
#else  //POSITIVE_APPROACH
  float r;
  //문턱값을 설정하지 않을 경우 매우 적은 표본으로 좋은 결과를 낸 다른 인코딩이 컨피던스가 1보다 높게나온다
  if (mTotalSeqs > thresh) {
    r = ((float)1.0) * mSeqCounters[POSITIVE_CAT] / mTotalSeqs / mModel->mTypicalPositiveRatio;
    /* Multiply by a ratio of positive sequences per characters.
     * This would help in particular to distinguish close winners.
     * Indeed if you add a letter, you'd expect the positive sequence count
     * to increase as well. If it doesn't, it may mean that this new codepoint
     * may not have been a letter, but instead a symbol (or some other
     * character). This could make the difference between very closely related
     * charsets used for the same language.
     */
    r = r * (mSeqCounters[POSITIVE_CAT] + (float) mSeqCounters[PROBABLE_CAT] / 4) / mTotalChar;
    /* The more control characters (proportionnaly to the size of the text), the
     * less confident we become in the current charset.
     */
    r = r * (mTotalChar - mCtrlChar) / mTotalChar;
    r = r*mFreqChar/mTotalChar;
    if (r >= (float)1.00)
      r = (float)0.99;
    return r;
  }
  return (float)0.01;
#endif
}

const char* nsSingleByteCharSetProber::GetCharSetName()
{
  if (!mNameProber)
    return mModel->charsetName;
  return mNameProber->GetCharSetName();
}

#ifdef DEBUG_chardet
void nsSingleByteCharSetProber::DumpStatus()
{
  printf("  SBCS: %1.3f [%s]\r\n", GetConfidence(), GetCharSetName());
}
#endif
