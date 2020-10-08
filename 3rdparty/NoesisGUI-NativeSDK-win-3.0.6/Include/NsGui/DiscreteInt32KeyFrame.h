////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __GUI_DISCRETEINT32KEYFRAME_H__
#define __GUI_DISCRETEINT32KEYFRAME_H__


#include <NsCore/Noesis.h>
#include <NsCore/ReflectionDeclare.h>
#include <NsGui/Int32KeyFrame.h>
#include <NsGui/AnimationApi.h>


namespace Noesis
{

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Animates from the *Int32* value of the previous key frame to its own *Value* using discrete
/// interpolation.
///
/// http://msdn.microsoft.com/en-us/library/system.windows.media.animation.discreteint32keyframe.aspx
////////////////////////////////////////////////////////////////////////////////////////////////////
class NS_GUI_ANIMATION_API DiscreteInt32KeyFrame final: public Int32KeyFrame
{
public:
    // Hides Freezable methods for convenience
    //@{
    Ptr<DiscreteInt32KeyFrame> Clone() const;
    Ptr<DiscreteInt32KeyFrame> CloneCurrentValue() const;
    //@}

protected:
    /// From Freezable
    //@{
    Ptr<Freezable> CreateInstanceCore() const override;
    //@}

    /// From KeyFrame
    //@{
    int32_t InterpolateValueCore(int32_t baseValue, float keyFrameProgress) override;
    //@}

    NS_DECLARE_REFLECTION(DiscreteInt32KeyFrame, Int32KeyFrame)
};

}


#endif
