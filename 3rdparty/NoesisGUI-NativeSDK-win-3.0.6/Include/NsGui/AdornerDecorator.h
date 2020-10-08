////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __GUI_ADORNERDECORATOR_H__
#define __GUI_ADORNERDECORATOR_H__


#include <NsCore/Noesis.h>
#include <NsGui/CoreApi.h>
#include <NsGui/Decorator.h>
#include <NsGui/ILayerManager.h>
#include <NsCore/ReflectionDeclare.h>
#include <NsCore/Ptr.h>


namespace Noesis
{

NS_WARNING_PUSH
NS_MSVC_WARNING_DISABLE(4251 4275)

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Provides an adorner layer for elements beneath it in the visual tree.
///
/// http://msdn.microsoft.com/en-us/library/system.windows.documents.adornerdecorator.aspx
////////////////////////////////////////////////////////////////////////////////////////////////////
class NS_GUI_CORE_API AdornerDecorator: public Decorator, public ILayerManager
{
public:
    /// Constructor
    AdornerDecorator(bool logicalChild = true);

    /// Destructor
    ~AdornerDecorator();

    /// From ILayerManager
    //@{
    void AddLayer(Visual* layerRoot) override;
    void RemoveLayer(Visual* layerRoot) override;
    //@}

    NS_IMPLEMENT_INTERFACE_FIXUP

protected:
    /// From DependencyObject
    //@{
    void OnInit() override;
    //@}

    /// From Visual
    //@{
    uint32_t GetVisualChildrenCount() const override;
    Visual* GetVisualChild(uint32_t index) const override;
    //@}

    /// From FrameworkElement
    //@{
    Size MeasureOverride(const Size& availableSize) override;
    Size ArrangeOverride(const Size& finalSize) override;
    //@}

    /// From Decorator
    //@{
    void OnChildChanged(UIElement* oldChild, UIElement* newChild) override;
    //@}

private:
    struct AdornerLayers;
    Ptr<AdornerLayers> mLayers;

    NS_DECLARE_REFLECTION(AdornerDecorator, Decorator);
};

NS_WARNING_POP

}

#endif
