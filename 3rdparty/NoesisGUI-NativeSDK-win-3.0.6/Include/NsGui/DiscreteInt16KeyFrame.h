////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __GUI_DISCRETEINT16KEYFRAME_H__
#define __GUI_DISCRETEINT16KEYFRAME_H__


#include <NsCore/Noesis.h>
#include <NsCore/ReflectionDeclare.h>
#include <NsGui/Int16KeyFrame.h>
#include <NsGui/AnimationApi.h>


namespace Noesis
{

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Animates from the *Int16* value of the previous key frame to its own *Value* using discrete
/// interpolation.
///
/// http://msdn.microsoft.com/en-us/library/system.windows.media.animation.discreteint16keyframe.aspx
////////////////////////////////////////////////////////////////////////////////////////////////////
class NS_GUI_ANIMATION_API DiscreteInt16KeyFrame final: public Int16KeyFrame
{
public:
    // Hides Freezable methods for convenience
    //@{
    Ptr<DiscreteInt16KeyFrame> Clone() const;
    Ptr<DiscreteInt16KeyFrame> CloneCurrentValue() const;
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

    NS_DECLARE_REFLECTION(DiscreteInt16KeyFrame, Int16KeyFrame)
};

}


#endif
