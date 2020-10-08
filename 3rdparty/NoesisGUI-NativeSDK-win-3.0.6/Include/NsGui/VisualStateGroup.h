////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __GUI_VISUALSTATEGROUP_H__
#define __GUI_VISUALSTATEGROUP_H__


#include <NsCore/Noesis.h>
#include <NsCore/ReflectionDeclare.h>
#include <NsGui/DependencyObject.h>
#include <NsGui/IUITreeNode.h>
#include <NsGui/AnimationApi.h>
#include <NsCore/String.h>
#include <NsCore/HashMap.h>


namespace Noesis
{

struct VisualStateChangedEventArgs;
class FrameworkElement;
class Storyboard;
class VisualState;
class VisualTransition;

template<class T> class UICollection;
typedef Noesis::UICollection<Noesis::VisualState> VisualStateCollection;
typedef Noesis::UICollection<Noesis::VisualTransition> VisualTransitionCollection;

NS_WARNING_PUSH
NS_MSVC_WARNING_DISABLE(4251 4275)

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Contains mutually exclusive VisualState objects and VisualTransition objects that are used
/// to go from one state to another.
///
/// http://msdn.microsoft.com/en-us/library/system.windows.visualstategroup.aspx
////////////////////////////////////////////////////////////////////////////////////////////////////
class NS_GUI_ANIMATION_API VisualStateGroup: public DependencyObject, public IUITreeNode
{
public:
    VisualStateGroup();
    ~VisualStateGroup();

    /// Gets the name of the VisualStateGroup.
    const char* GetName() const;

    /// Gets the collection of mutually exclusive VisualState objects
    VisualStateCollection* GetStates() const;

    /// Gets the collection of VisualTransition objects
    VisualTransitionCollection* GetTransitions() const;

    /// Gets or the state that is currently active
    //@{
    VisualState* GetCurrentState(FrameworkElement* fe) const;
    void SetCurrentState(FrameworkElement* fe, VisualState* state);
    //@}

    /// Finds a matching state with the same name (can be null or empty)
    VisualState* FindState(Symbol name) const;

    /// Finds a matching transition for the provided *from* and "to* VisualState objects
    VisualTransition* FindTransition(VisualState* from, VisualState* to) const;

    /// Updates element animations with the specified storyboards
    void UpdateAnimations(FrameworkElement* fe, Storyboard* storyboard1,
        Storyboard* storyboard2 = 0);

    /// Creates a Storyboard that animates changes from current state to the specifed new state
    Ptr<Storyboard> CreateTransitionStoryboard(FrameworkElement* root, VisualState* newState,
        VisualTransition* transition);

    /// From IUITreeNode
    //@{
    IUITreeNode* GetNodeParent() const override;
    void SetNodeParent(IUITreeNode* parent) override;
    BaseComponent* FindNodeResource(const char* key, bool fullElementSearch) const override;
    BaseComponent* FindNodeName(const char* name) const override;
    ObjectWithNameScope FindNodeNameAndScope(const char* name) const override;
    //@}

    NS_IMPLEMENT_INTERFACE_FIXUP

protected:
    /// From DependencyObject
    //@{
    void OnInit() override;
    void OnObjectValueSet(BaseComponent* oldValue, BaseComponent* newValue) override;
    //@}

private:
    void EnsureStates() const;
    void EnsureTransitions() const;

    void OnObjectDestroyed(DependencyObject* object);

private:
    String mName;
    mutable Ptr<VisualStateCollection> mStates;
    mutable Ptr<VisualTransitionCollection> mTransitions;

    struct ElementState
    {
        Ptr<VisualState> state;
        Ptr<Storyboard> storyboards[2];
    };

    typedef HashMap<FrameworkElement*, ElementState> CurrentStates;
    CurrentStates mCurrentStates;

    IUITreeNode* mOwner;

    NS_DECLARE_REFLECTION(VisualStateGroup, DependencyObject)
};

NS_WARNING_POP

}


#endif
