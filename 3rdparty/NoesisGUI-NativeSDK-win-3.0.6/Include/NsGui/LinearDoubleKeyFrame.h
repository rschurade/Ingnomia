////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __GUI_LINEARDOUBLEKEYFRAME_H__
#define __GUI_LINEARDOUBLEKEYFRAME_H__


#include <NsCore/Noesis.h>
#include <NsCore/ReflectionDeclare.h>
#include <NsGui/DoubleKeyFrame.h>
#include <NsGui/AnimationApi.h>


namespace Noesis
{

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Animates from the *Float* value of the previous key frame to its own *Value* using linear
/// interpolation.
///
/// http://msdn.microsoft.com/en-us/library/system.windows.media.animation.lineardoublekeyframe.aspx
////////////////////////////////////////////////////////////////////////////////////////////////////
class NS_GUI_ANIMATION_API LinearDoubleKeyFrame final: public DoubleKeyFrame
{
public:
    // Hides Freezable methods for convenience
    //@{
    Ptr<LinearDoubleKeyFrame> Clone() const;
    Ptr<LinearDoubleKeyFrame> CloneCurrentValue() const;
    //@}

protected:
    /// From Freezable
    //@{
    Ptr<Freezable> CreateInstanceCore() const override;
    //@}

    /// From KeyFrame
    //@{
    float InterpolateValueCore(float baseValue, float keyFrameProgress) override;
    //@}

    NS_DECLARE_REFLECTION(LinearDoubleKeyFrame, DoubleKeyFrame)
};

}


#endif
