////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __GUI_PEN_H__
#define __GUI_PEN_H__


#include <NsCore/Noesis.h>
#include <NsGui/CoreApi.h>
#include <NsGui/Animatable.h>
#include <NsGui/IRenderProxyCreator.h>


namespace Noesis
{

struct Thickness;
class Brush;
class DashStyle;
enum PenLineCap: int32_t;
enum PenLineJoin: int32_t;

NS_WARNING_PUSH
NS_MSVC_WARNING_DISABLE(4251 4275)

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Describes how a shape is outlined.
///
/// http://msdn.microsoft.com/en-us/library/system.windows.media.pen.aspx
////////////////////////////////////////////////////////////////////////////////////////////////////
class NS_GUI_CORE_API Pen final: public Animatable, public IRenderProxyCreator
{
public:
    Pen();
    ~Pen();

    /// Gets or sets the Brush used to fill the outline produced by this Pen
    //@{
    Brush* GetBrush() const;
    void SetBrush(Brush* brush);
    //@}

    /// Gets or sets a value that specifies how the ends of each dash are drawn
    //@{
    PenLineCap GetDashCap() const;
    void SetDashCap(PenLineCap cap);
    //@}

    /// Gets or sets a value that describes the pattern of dashes generated by this Pen
    //@{
    DashStyle* GetDashStyle() const;
    void SetDashStyle(DashStyle* dasheStyle);
    //@}

    /// Gets or sets the type of shape to use at the end of a stroke
    //@{
    PenLineCap GetEndLineCap() const;
    void SetEndLineCap(PenLineCap endCap);
    //@}

    /// Gets or sets the type of joint used at the vertices of a shape's outline
    //@{
    PenLineJoin GetLineJoin() const;
    void SetLineJoin(PenLineJoin join);
    //@}

    /// Gets or sets the limit on the ratio of the miter length to half this pen's Thickness
    //@{
    float GetMiterLimit() const;
    void SetMiterLimit(float limit);
    //@}

    /// Gets or sets the type of shape to use at the beginning of a stroke
    //@{
    PenLineCap GetStartLineCap() const;
    void SetStartLineCap(PenLineCap startCap);
    //@}

    /// Gets or sets the thickness of the stroke produced by this Pen
    //@{
    float GetThickness() const;
    void SetThickness(float thickness);
    //@}

    /// Gets or sets the amount to trim the start of the geometry path
    //@{
    float GetTrimStart() const;
    void SetTrimStart(float value);
    //@}

    /// Gets or sets the amount to trim the end of the geometry path
    //@{
    float GetTrimEnd() const;
    void SetTrimEnd(float value);
    //@}

    /// Gets or sets the amount to offset trimming the geometry path
    //@{
    float GetTrimOffset() const;
    void SetTrimOffset(float value);
    //@}

    /// Indicates if pen is renderable
    bool IsRenderable() const;

    // Hides Freezable methods for convenience
    //@{
    Ptr<Pen> Clone() const;
    Ptr<Pen> CloneCurrentValue() const;
    //@}

    /// From IRenderProxyCreator
    //@{
    void CreateRenderProxy(RenderTreeUpdater& updater, uint32_t proxyIndex) override;
    void UpdateRenderProxy(RenderTreeUpdater& updater, uint32_t proxyIndex) override;
    void UnregisterRenderer(ViewId viewId) override;
    //@}

    NS_IMPLEMENT_INTERFACE_FIXUP
    
public:
    /// Dependency properties
    //@{
    static const DependencyProperty* BrushProperty;
    static const DependencyProperty* DashCapProperty;
    static const DependencyProperty* DashStyleProperty;
    static const DependencyProperty* EndLineCapProperty;
    static const DependencyProperty* LineJoinProperty;
    static const DependencyProperty* MiterLimitProperty;
    static const DependencyProperty* StartLineCapProperty;
    static const DependencyProperty* ThicknessProperty;
    static const DependencyProperty* TrimStartProperty;
    static const DependencyProperty* TrimEndProperty;
    static const DependencyProperty* TrimOffsetProperty;
    //@}

protected:
    /// From DependencyObject
    //@{
    bool OnPropertyChanged(const DependencyPropertyChangedEventArgs& args) override;
    bool OnSubPropertyChanged(const DependencyProperty* dp) override;
    //@}

    /// From Freezable
    //@{
    Ptr<Freezable> CreateInstanceCore() const override;
    //@}

private:
    RenderProxyCreatorFlags mUpdateFlags;

    enum UpdateFlags
    {
        UpdateFlags_Brush,
        UpdateFlags_Thickness,
        UpdateFlags_DashStyle,
        UpdateFlags_DashCap,
        UpdateFlags_StartLineCap,
        UpdateFlags_EndLineCap,
        UpdateFlags_LineJoin,
        UpdateFlags_MiterLimit,
        UpdateFlags_TrimStart,
        UpdateFlags_TrimEnd,
        UpdateFlags_TrimOffset
    };

    NS_DECLARE_REFLECTION(Pen, Animatable)
};

NS_WARNING_POP

}


#endif
