/*
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is Mozilla MathML Project.
 * 
 * The Initial Developer of the Original Code is The University Of
 * Queensland.  Portions created by The University Of Queensland are
 * Copyright (C) 1999 The University Of Queensland.  All Rights Reserved.
 * 
 * Contributor(s):
 *   Roger B. Sidje <rbs@maths.uq.edu.au>
 *   David J. Fiddes <D.J.Fiddes@hw.ac.uk>
 *   Shyjan Mahamud <mahamud@cs.cmu.edu> (added TeX rendering rules)
 */


#include "nsCOMPtr.h"
#include "nsHTMLParts.h"
#include "nsIHTMLContent.h"
#include "nsFrame.h"
#include "nsLineLayout.h"
#include "nsHTMLIIDs.h"
#include "nsIPresContext.h"
#include "nsHTMLAtoms.h"
#include "nsUnitConversion.h"
#include "nsIStyleContext.h"
#include "nsStyleConsts.h"
#include "nsINameSpaceManager.h"
#include "nsIRenderingContext.h"
#include "nsIFontMetrics.h"
#include "nsStyleUtil.h"

#include "nsMathMLmsubsupFrame.h"

//
// <msubsup> -- attach a subscript-superscript pair to a base - implementation
//

nsresult
NS_NewMathMLmsubsupFrame(nsIPresShell* aPresShell, nsIFrame** aNewFrame)
{
  NS_PRECONDITION(aNewFrame, "null OUT ptr");
  if (nsnull == aNewFrame) {
    return NS_ERROR_NULL_POINTER;
  }
  nsMathMLmsubsupFrame* it = new (aPresShell) nsMathMLmsubsupFrame;
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  *aNewFrame = it;
  return NS_OK;
}

nsMathMLmsubsupFrame::nsMathMLmsubsupFrame()
{
}

nsMathMLmsubsupFrame::~nsMathMLmsubsupFrame()
{
}

NS_IMETHODIMP
nsMathMLmsubsupFrame::Init(nsIPresContext*  aPresContext,
         nsIContent*      aContent,
         nsIFrame*        aParent,
         nsIStyleContext* aContext,
         nsIFrame*        aPrevInFlow)
{
  nsresult rv = nsMathMLContainerFrame::Init
    (aPresContext, aContent, aParent, aContext, aPrevInFlow);

  mSubScriptShift = 0;
  mSupScriptShift = 0;
  mScriptSpace = 0;

  // check if the subscriptshift attribute is there
  nsAutoString value;
  if (NS_CONTENT_ATTR_HAS_VALUE == GetAttribute(mContent, mPresentationData.mstyle,
                   nsMathMLAtoms::subscriptshift_, value)) {
    nsCSSValue cssValue;
    if (ParseNumericValue(value, cssValue) && cssValue.IsLengthUnit()) {
      mSubScriptShift = CalcLength(aPresContext, mStyleContext, cssValue);
    }
  }
  // check if the superscriptshift attribute is there
  if (NS_CONTENT_ATTR_HAS_VALUE == GetAttribute(mContent, mPresentationData.mstyle,
                   nsMathMLAtoms::superscriptshift_, value)) {
    nsCSSValue cssValue;
    if (ParseNumericValue(value, cssValue) && cssValue.IsLengthUnit()) {
      mSupScriptShift = CalcLength(aPresContext, mStyleContext, cssValue);
    }
  }

#if defined(NS_DEBUG) && defined(SHOW_BOUNDING_BOX)
  mPresentationData.flags |= NS_MATHML_SHOW_BOUNDING_METRICS;
#endif
  return rv;
}


NS_IMETHODIMP
nsMathMLmsubsupFrame::Place (nsIPresContext*      aPresContext,
                             nsIRenderingContext& aRenderingContext,
                             PRBool               aPlaceOrigin,
                             nsHTMLReflowMetrics& aDesiredSize)
{
  return nsMathMLmsubsupFrame::PlaceSubSupScript (aPresContext,
                                                  aRenderingContext,
                                                  aPlaceOrigin,
                                                  aDesiredSize,
                                                  this,
                                                  mSubScriptShift,
                                                  mScriptSpace);
}

// exported routine that both munderover and msubsup share.
// munderover uses this when movablelimits is set.
nsresult
nsMathMLmsubsupFrame::PlaceSubSupScript (nsIPresContext*      aPresContext,
                                         nsIRenderingContext& aRenderingContext,
                                         PRBool               aPlaceOrigin,
                                         nsHTMLReflowMetrics& aDesiredSize,
                                         nsIFrame*            aFrame,
                                         nscoord              aUserSubScriptShift,
                                         nscoord              aUserSupScriptShift,
                                         nscoord              aScriptSpace)
{
  nsresult rv = NS_OK;

  // the caller better be a mathml frame
  nsIMathMLFrame* mathMLFrame = nsnull;
  rv = aFrame->QueryInterface (NS_GET_IID(nsIMathMLFrame),
                                     (void**)&mathMLFrame);
  if (NS_FAILED(rv) || !mathMLFrame) return rv;

  // force the scriptSpace to be atleast 1 pixel
  float p2t;
  aPresContext->GetScaledPixelsToTwips(&p2t);
  aScriptSpace = PR_MAX(NSIntPixelsToTwips(1, p2t), aScriptSpace);

  ////////////////////////////////////
  // Get the children's desired sizes

  PRInt32 count = 0;
  nsHTMLReflowMetrics baseSize (nsnull);
  nsHTMLReflowMetrics subScriptSize (nsnull);
  nsHTMLReflowMetrics supScriptSize (nsnull);
  nsIFrame* baseFrame = nsnull;
  nsIFrame* subScriptFrame = nsnull;
  nsIFrame* supScriptFrame = nsnull;
  // parameter v, Rule 18a, Appendix G of the TeXbook
  nscoord minSubScriptShift = 0;
  // parameter u in Rule 18a, Appendix G of the TeXbook
  nscoord minSupScriptShift = 0;

  nsBoundingMetrics bmBase, bmSubScript, bmSupScript;

  nsIFrame* aChildFrame = nsnull;
  rv = aFrame->FirstChild (aPresContext, nsnull, &aChildFrame);
  if (!NS_SUCCEEDED(rv) || (nsnull == aChildFrame)) {
    return rv;
  }
  while (nsnull != aChildFrame) {
    if (!IsOnlyWhitespace(aChildFrame)) {
      if (0 == count) {
        // base
        baseFrame = aChildFrame;
        GetReflowAndBoundingMetricsFor(baseFrame, baseSize, bmBase);
      }
      else if (1 == count) {
        // subscript
        subScriptFrame = aChildFrame;
        GetReflowAndBoundingMetricsFor(subScriptFrame, subScriptSize, bmSubScript);
        // get the subdrop from the subscript font
        nscoord aSubDrop;
        GetSubDropFromChild (aPresContext, subScriptFrame, aSubDrop);
        // parameter v, Rule 18a, App. G, TeXbook
        minSubScriptShift = bmBase.descent + aSubDrop;
      }
      else if (2 == count) {
        // superscript
        supScriptFrame = aChildFrame;
        GetReflowAndBoundingMetricsFor(supScriptFrame, supScriptSize, bmSupScript);
        // get the supdrop from the supscript font
        nscoord aSupDrop;
        GetSupDropFromChild (aPresContext, supScriptFrame, aSupDrop);
        // parameter u, Rule 18a, App. G, TeXbook
        minSupScriptShift = bmBase.ascent - aSupDrop;
      }
      count++;
    }
    aChildFrame->GetNextSibling(&aChildFrame);
  }
#ifdef NS_DEBUG
  if (3 != count) printf("msubsup: invalid markup\n");
#endif
  if ((3 != count) || !baseFrame || !subScriptFrame || !supScriptFrame) {
    // report an error, encourage people to get their markups in order
    return NS_STATIC_CAST(nsMathMLContainerFrame*,
                          aFrame)->ReflowError(aPresContext,
                                               aRenderingContext,
                                               aDesiredSize);
  }

  //////////////////
  // Place Children
  //////////////////

  //////////////////////////////////////////////////
  // Get subscript shift
  // slightly different from nsMathMLmsubFrame.cpp
  //////////////////////////////////////////////////

  // aSubScriptShift{1,2}
  // = minimum amount to shift the subscript down
  // = sub{1,2} in TeXbook
  // aSubScriptShift1 = subscriptshift attribute * x-height
  nscoord aSubScriptShift1, aSubScriptShift2;

//  const nsStyleFont* aFont =
//    (const nsStyleFont*) mStyleContext->GetStyleData (eStyleStruct_Font);

  const nsStyleFont* font;
  baseFrame->GetStyleData(eStyleStruct_Font, (const nsStyleStruct *&)font);
  aRenderingContext.SetFont(font->mFont);
  nsCOMPtr<nsIFontMetrics> fm;
  aRenderingContext.GetFontMetrics(*getter_AddRefs(fm));

  // get x-height (an ex)
  nscoord xHeight;
  fm->GetXHeight (xHeight);

  nscoord aRuleSize;
  GetRuleThickness (aRenderingContext, fm, aRuleSize);

  // Get aSubScriptShift{1,2} default from font
  GetSubScriptShifts (fm, aSubScriptShift1, aSubScriptShift2);

  if (0 < aUserSubScriptShift) {
    // the user has set the subscriptshift attribute
    float aFactor = ((float) aSubScriptShift2) / aSubScriptShift1;
    aSubScriptShift1 = PR_MAX(aSubScriptShift1, aUserSubScriptShift);
    aSubScriptShift2 = NSToCoordRound(aFactor * aSubScriptShift1);
  }

  // get a tentative value for subscriptshift
  // Rule 18d, App. G, TeXbook
  nscoord aSubScriptShift =
    PR_MAX(minSubScriptShift,PR_MAX(aSubScriptShift1,aSubScriptShift2));

  //////////////////////////////////////////////////
  // Get supscript shift
  // same code from nsMathMLmsupFrame.cpp
  //////////////////////////////////////////////////

  // get min supscript shift limit from x-height
  // = d(x) + 1/4 * sigma_5, Rule 18c, App. G, TeXbook
  nscoord minShiftFromXHeight = (nscoord)
    (bmSupScript.descent + (1.0f/4.0f) * xHeight);

  // aSupScriptShift{1,2,3}
  // = minimum amount to shift the supscript up
  // = sup{1,2,3} in TeX
  // aSupScriptShift1 = superscriptshift attribute * x-height
  // Note that there are THREE values for supscript shifts depending
  // on the current style
  nscoord aSupScriptShift1, aSupScriptShift2, aSupScriptShift3;
  // Set aSupScriptShift{1,2,3} default from font
  GetSupScriptShifts (fm, aSupScriptShift1, aSupScriptShift2, aSupScriptShift3);
  if (0 < aUserSupScriptShift) {
    // the user has set the superscriptshift attribute
    float aFactor2 = ((float) aSupScriptShift2) / aSupScriptShift1;
    float aFactor3 = ((float) aSupScriptShift3) / aSupScriptShift1;
    aSupScriptShift1 = PR_MAX(aSupScriptShift1, aUserSupScriptShift);
    aSupScriptShift2 = NSToCoordRound(aFactor2 * aSupScriptShift1);
    aSupScriptShift3 = NSToCoordRound(aFactor3 * aSupScriptShift1);
  }

  // get sup script shift depending on current script level and display style
  // Rule 18c, App. G, TeXbook
  nscoord aSupScriptShift;
  nsPresentationData presentationData;
  mathMLFrame->GetPresentationData(presentationData);
  if ( presentationData.scriptLevel == 0 &&
       NS_MATHML_IS_DISPLAYSTYLE(presentationData.flags) &&
      !NS_MATHML_IS_COMPRESSED(presentationData.flags)) {
    // Style D in TeXbook
    aSupScriptShift = aSupScriptShift1;
  }
  else if (NS_MATHML_IS_COMPRESSED(presentationData.flags)) {
    // Style C' in TeXbook = D',T',S',SS'
    aSupScriptShift = aSupScriptShift3;
  }
  else {
    // everything else = T,S,SS
    aSupScriptShift = aSupScriptShift2;
  }

  // get tentative value for superscriptshift
  // Rule 18c, App. G, TeXbook
  aSupScriptShift =
    PR_MAX(minSupScriptShift,PR_MAX(aSupScriptShift,minShiftFromXHeight));

  //////////////////////////////////////////////////
  // Negotiate between aSupScriptShift and aSubScriptShift
  // so that there will be enough gap between them
  // Rule 18e, App. G, TeXbook
  //////////////////////////////////////////////////

  nscoord gap =
    (aSupScriptShift - bmSupScript.descent) -
    (bmSubScript.ascent - aSubScriptShift);
  if (gap < 4.0f * aRuleSize) {
    // adjust aSubScriptShift to get a gap of (4.0 * aRuleSize)
    aSubScriptShift += NSToCoordRound ((4.0f * aRuleSize) - gap);
  }

  // next we want to ensure that the bottom of the superscript
  // will be > (4/5) * x-height above baseline
  gap = NSToCoordRound ((4.0f/5.0f) * xHeight -
                        (aSupScriptShift - bmSupScript.descent));
  if (gap > 0.0f) {
    aSupScriptShift += gap;
    aSubScriptShift -= gap;
  }

  //////////////////////////////////////////////////
  // Do the Placing
  //////////////////////////////////////////////////

  // get bounding box for base + subscript + superscript
  nsBoundingMetrics boundingMetrics;
  boundingMetrics.ascent =
    PR_MAX(bmBase.ascent, (bmSupScript.ascent + aSupScriptShift));
  boundingMetrics.descent =
   PR_MAX(bmBase.descent, (bmSubScript.descent + aSubScriptShift));

  // add aScriptSpace to both super/subscript
  // add italicCorrection only to superscript
  nscoord italicCorrection;
  GetItalicCorrection(bmBase, italicCorrection);
  boundingMetrics.width = bmBase.width + aScriptSpace +
    PR_MAX((italicCorrection + bmSupScript.width), bmSubScript.width);
  boundingMetrics.leftBearing = bmBase.leftBearing;
  boundingMetrics.rightBearing = bmBase.width + aScriptSpace +
    PR_MAX((italicCorrection + bmSupScript.rightBearing), bmSubScript.rightBearing);
  mathMLFrame->SetBoundingMetrics(boundingMetrics);

  // reflow metrics
  aDesiredSize.ascent =
    PR_MAX(baseSize.ascent, 
       PR_MAX(subScriptSize.ascent - aSubScriptShift,
              supScriptSize.ascent + aSupScriptShift));
  aDesiredSize.descent =
    PR_MAX(baseSize.descent,
       PR_MAX(subScriptSize.descent + aSubScriptShift, 
              supScriptSize.descent - aSupScriptShift));
  aDesiredSize.height = aDesiredSize.ascent + aDesiredSize.descent;
  aDesiredSize.width = bmBase.width + aScriptSpace +
    PR_MAX((italicCorrection + supScriptSize.width), subScriptSize.width);
  aDesiredSize.mBoundingMetrics = boundingMetrics;

  mathMLFrame->SetReference(nsPoint(0, aDesiredSize.ascent));

  if (aPlaceOrigin) {
    nscoord dx, dy;
    // now place the base ...
    dx = 0; dy = aDesiredSize.ascent - baseSize.ascent;
    FinishReflowChild(baseFrame, aPresContext, baseSize, dx, dy, 0);
    // ... and subscript
    dx = bmBase.width + aScriptSpace;
    dy = aDesiredSize.ascent - (subScriptSize.ascent - aSubScriptShift);
    FinishReflowChild(subScriptFrame, aPresContext, subScriptSize, dx, dy, 0);
    // ... and the superscript
    dx = bmBase.width + aScriptSpace + italicCorrection;
    dy = aDesiredSize.ascent - (supScriptSize.ascent + aSupScriptShift);
    FinishReflowChild(supScriptFrame, aPresContext, supScriptSize, dx, dy, 0);
  }

  return NS_OK;
}
