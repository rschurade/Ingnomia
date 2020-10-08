////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __GUI_SPLINEDOUBLEKEYFRAME_H__
#define __GUI_SPLINEDOUBLEKEYFRAME_H__


#include <NsCore/Noesis.h>
#include <NsCore/ReflectionDeclare.h>
#include <NsGui/DoubleKeyFrame.h>
#include <NsGui/AnimationApi.h>


namespace Noesis
{

class KeySpline;

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Animates from the *Float* value of the previous key frame to its own *Value* using splined 
/// interpolation.
///
/// http://msdn.microsoft.com/en-us/library/system.windows.media.animation.splinedoublekeyframe.aspx
////////////////////////////////////////////////////////////////////////////////////////////////////
class NS_GUI_ANIMATION_API SplineDoubleKeyFrame final: public DoubleKeyFrame
{
public:
    /// Gets or sets the two control points that define animation progress for this key frame
    //@{
    KeySpline* GetKeySpline() const;
    void SetKeySpline(KeySpline* spline);
    //@}

    // Hides Freezable methods for convenience
    //@{
    Ptr<SplineDoubleKeyFrame> Clone() const;
    Ptr<SplineDoubleKeyFrame> CloneCurrentValue() const;
    //@}

public:
    /// Dependency properties
    //@{
    static const DependencyProperty* KeySplineProperty;
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

    NS_DECLARE_REFLECTION(SplineDoubleKeyFrame, DoubleKeyFrame)
};

}


#endif
