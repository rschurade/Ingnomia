////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __GUI_SPLINEINT16KEYFRAME_H__
#define __GUI_SPLINEINT16KEYFRAME_H__


#include <NsCore/Noesis.h>
#include <NsCore/ReflectionDeclare.h>
#include <NsGui/Int16KeyFrame.h>
#include <NsGui/AnimationApi.h>


namespace Noesis
{

class KeySpline;

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Animates from the *Int16* value of the previous key frame to its own *Value* using splined 
/// interpolation.
///
/// http://msdn.microsoft.com/en-us/library/system.windows.media.animation.splineint16keyframe.aspx
////////////////////////////////////////////////////////////////////////////////////////////////////
class NS_GUI_ANIMATION_API SplineInt16KeyFrame final: public Int16KeyFrame
{
public:
    /// Gets or sets the two control points that define animation progress for this key frame
    //@{
    KeySpline* GetKeySpline() const;
    void SetKeySpline(KeySpline* spline);
    //@}

    // Hides Freezable methods for convenience
    //@{
    Ptr<SplineInt16KeyFrame> Clone() const;
    Ptr<SplineInt16KeyFrame> CloneCurrentValue() const;
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
    int16_t InterpolateValueCore(int16_t baseValue, float keyFrameProgress) override;
    //@}

    NS_DECLARE_REFLECTION(SplineInt16KeyFrame, Int16KeyFrame)
};

}


#endif
