////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __GUI_DRAWINGCONTEXT_H__
#define __GUI_DRAWINGCONTEXT_H__


#include <NsCore/Noesis.h>
#include <NsGui/CoreApi.h>
#include <NsCore/BaseComponent.h>
#include <NsCore/ReflectionDeclare.h>
#include <NsCore/Ptr.h>

// hack
#undef DrawText


namespace Noesis
{

struct Point;
struct Rect;
struct Size;
struct Color;
class Brush;
class Pen;
class Geometry;
class ImageSource;
class FormattedText;
class Transform;
class DrawingCommands;
template<class T> class Delegate;

NS_WARNING_PUSH
NS_MSVC_WARNING_DISABLE(4251 4275)

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Describes visual content using draw, push, and pop commands during UIElement *OnRender*.
////////////////////////////////////////////////////////////////////////////////////////////////////
class NS_GUI_CORE_API DrawingContext: public BaseComponent
{
public:
    DrawingContext();
    ~DrawingContext();

    /// Draws the specified Geometry using the specified Brush and Pen
    void DrawGeometry(Brush* brush, Pen* pen, Geometry* geometry);

    /// Draws an image into the region defined by the specified Rect
    void DrawImage(ImageSource* imageSource, const Rect& rect);

    /// Draws formatted text at the specified location
    void DrawText(FormattedText* formattedText, const Rect& bounds);

    /// Pops the last opacity mask, opacity, clip, effect, or transform operation that was pushed
    /// onto the drawing context
    void Pop();

    /// Pushes the specified clip region onto the drawing context
    void PushClip(Geometry* clipGeometry);

    /// Pushes the specified Transform onto the drawing context
    void PushTransform(Transform* transform);

private:
    friend class UIElement;
    friend class DrawingContextTestBase;
    DrawingCommands* GetDrawingCommands() const;
    void EnsureDrawingCommands();
    void ResetDrawingCommands();

private:
    Ptr<DrawingCommands> mCommands;
    uint32_t mPushes;

    NS_DECLARE_REFLECTION(DrawingContext, BaseComponent)
};

NS_WARNING_POP

}

#endif
