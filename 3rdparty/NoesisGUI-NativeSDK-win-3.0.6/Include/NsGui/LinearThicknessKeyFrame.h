////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __GUI_LINEARTHICKNESSKEYFRAME_H__
#define __GUI_LINEARTHICKNESSKEYFRAME_H__


#include <NsCore/Noesis.h>
#include <NsCore/ReflectionDeclare.h>
#include <NsGui/ThicknessKeyFrame.h>
#include <NsGui/AnimationApi.h>


namespace Noesis
{

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Animates from the *Thickness* value of the previous key frame to its own *Value* using linear
/// interpolation.
///
/// http://msdn.microsoft.com/en-us/library/system.windows.media.animation.linearthicknesskeyframe.aspx
////////////////////////////////////////////////////////////////////////////////////////////////////
class NS_GUI_ANIMATION_API LinearThicknessKeyFrame final: public ThicknessKeyFrame
{
public:
    // Hides Freezable methods for convenience
    //@{
    Ptr<LinearThicknessKeyFrame> Clone() const;
    Ptr<LinearThicknessKeyFrame> CloneCurrentValue() const;
    //@}

protected:
    /// From Freezable
    //@{
    Ptr<Freezable> CreateInstanceCore() const override;
    //@}

    /// From KeyFrame
    //@{
    Thickness InterpolateValueCore(const Thickness& baseValue, float keyFrameProgress) override;
    //@}

    NS_DECLARE_REFLECTION(LinearThicknessKeyFrame, ThicknessKeyFrame)
};

}


#endif
