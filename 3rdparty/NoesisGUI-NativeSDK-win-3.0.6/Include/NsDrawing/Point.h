////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __DRAWING_POINT_H__
#define __DRAWING_POINT_H__


#include <NsCore/Noesis.h>
#include <NsCore/ReflectionImplementEmpty.h>
#include <NsDrawing/TypesApi.h>
#include <NsDrawing/Size.h>
#include <NsMath/Vector.h>


namespace Noesis
{

struct Pointi;

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Point. Represents an x- and y-coordinate pair in two-dimensional space.
////////////////////////////////////////////////////////////////////////////////////////////////////
struct Point: public Vector2
{
    /// Default constructor that creates a (0,0) point
    Point();

    /// Constructor for x, y
    Point(float x, float y);

    /// Construct from Pointi
    Point(const Pointi& point);

    /// Constructor from Vector
    Point(const Vector2& v);

    /// Constructor from size
    explicit Point(const Size& size);

    /// Copy constructor
    Point(const Point& point) = default;

    /// Copy operator
    Point& operator=(const Point& point) = default;

    /// Generates a string representation of the point
    /// The string has the following form: "x,y"
    NS_DRAWING_TYPES_API String ToString() const;

    /// Tries to parse a Point from a string
    NS_DRAWING_TYPES_API static bool TryParse(const char* str, Point& result);

    NS_IMPLEMENT_INLINE_REFLECTION_(Point, Vector2)
};

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Pointi. Represents an x- and y-coordinate pair in integer two-dimensional space.
////////////////////////////////////////////////////////////////////////////////////////////////////
struct Pointi
{
    int x, y;

    /// Default constructor that creates a (0,0) point
    Pointi();

    /// Constructor for x, y
    Pointi(int x, int y);

    /// Constructor from float Point
    Pointi(const Point& p);

    /// Constructor from size
    explicit Pointi(const Sizei& size);

    /// Copy constructor
    Pointi(const Pointi& point) = default;

    /// Copy operator
    Pointi& operator=(const Pointi& point) = default;

    /// Generates a string representation of the point
    /// The string has the following form: "x,y"
    NS_DRAWING_TYPES_API String ToString() const;

    /// Tries to parse a Pointi from a string
    NS_DRAWING_TYPES_API static bool TryParse(const char* str, Pointi& result);

    NS_IMPLEMENT_INLINE_REFLECTION_(Pointi, NoParent)
};

}

#include <NsDrawing/Point.inl>

#endif
