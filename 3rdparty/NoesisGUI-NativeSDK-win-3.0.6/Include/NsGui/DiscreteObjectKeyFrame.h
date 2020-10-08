////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __GUI_DISCRETEOBJECTKEYFRAME_H__
#define __GUI_DISCRETEOBJECTKEYFRAME_H__


#include <NsCore/Noesis.h>
#include <NsCore/ReflectionDeclare.h>
#include <NsGui/ObjectKeyFrame.h>
#include <NsGui/AnimationApi.h>


namespace Noesis
{

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Animates from the *Object* value of the previous key frame to its own *Value* using discrete
/// interpolation.
///
/// http://msdn.microsoft.com/en-us/library/system.windows.media.animation.discreteobjectkeyframe.aspx
////////////////////////////////////////////////////////////////////////////////////////////////////
class NS_GUI_ANIMATION_API DiscreteObjectKeyFrame final: public ObjectKeyFrame
{
public:
    // Hides Freezable methods for convenience
    //@{
    Ptr<DiscreteObjectKeyFrame> Clone() const;
    Ptr<DiscreteObjectKeyFrame> CloneCurrentValue() const;
    //@}

protected:
    /// From Freezable
    //@{
    Ptr<Freezable> CreateInstanceCore() const override;
    //@}

    /// From KeyFrame
    //@{
    Ptr<BaseComponent> InterpolateValueCore(const Ptr<BaseComponent>& baseValue,
        float keyFrameProgress) override;
    //@}

    NS_DECLARE_REFLECTION(DiscreteObjectKeyFrame, ObjectKeyFrame)
};

}


#endif
