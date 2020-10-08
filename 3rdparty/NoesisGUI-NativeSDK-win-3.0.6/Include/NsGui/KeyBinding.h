////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __GUI_KEYBINDING_H__
#define __GUI_KEYBINDING_H__


#include <NsCore/Noesis.h>
#include <NsGui/CoreApi.h>
#include <NsGui/InputBinding.h>


namespace Noesis
{

class KeyGesture;
enum Key: int32_t;
enum ModifierKeys: int32_t;

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Binds a KeyGesture to a Command.
///
/// http://msdn.microsoft.com/en-us/library/system.windows.input.keybinding.aspx
////////////////////////////////////////////////////////////////////////////////////////////////////
class NS_GUI_CORE_API KeyBinding: public InputBinding
{
public:
    KeyBinding();
    KeyBinding(ICommand* command, KeyGesture* gesture);
    KeyBinding(ICommand* command, Key key, ModifierKeys modifiers);
    ~KeyBinding();

    /// Gets or sets the Key of the KeyGesture associated with this KeyBinding
    //@{
    Key GetKey() const;
    void SetKey(Key key);
    //@}

    /// Gets the modifier keys (one or more Alt, Ctrl, Shift) of the associated KeyGesture
    //@{
    ModifierKeys GetModifiers() const;
    void SetModifiers(ModifierKeys modifiers);
    //@}

    // Hides Freezable methods for convenience
    //@{
    Ptr<KeyBinding> Clone() const;
    Ptr<KeyBinding> CloneCurrentValue() const;
    //@}

public:
    static const DependencyProperty* KeyProperty;
    static const DependencyProperty* ModifiersProperty;

protected:
    /// From Freezable
    //@{
    Ptr<Freezable> CreateInstanceCore() const override;
    //@}

    /// From InputBinding
    //@{
    void OnGestureChanged(InputGesture* oldGesture, InputGesture* newGesture) override;
    //@}

private:
    void UpdateKeyAndModifers(KeyGesture* keyGesture);
    void UpdateGesture(Key key, ModifierKeys modifiers);

    NS_DECLARE_REFLECTION(KeyBinding, InputBinding)
};

}


#endif
