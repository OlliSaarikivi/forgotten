/*
* This is adapted from the iPhone port of Box2D. The original licence notice
* is below.
*/
/*
* Copyright (c) 2006-2007 Erin Catto http://www.gphysics.com
*
* iPhone port by Simon Oliver - http://www.simonoliver.com - http://www.handcircus.com
*
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
* 1. The origin of this software must not be misrepresented; you must not
* claim that you wrote the original software. If you use this software
* in a product, an acknowledgment in the product documentation would be
* appreciated but is not required.
* 2. Altered source versions must be plainly marked as such, and must not be
* misrepresented as being the original software.
* 3. This notice may not be removed or altered from any source distribution.
*/

#include "stdafx.h"
#include "Box2DGLDebugDraw.h"

Box2DGLDebugDraw::Box2DGLDebugDraw(b2World& world, View& view) : world(world), view(view)
{
    world.SetDebugDraw(this);

    vertex_shader.Source(
        "#version 140\n"
        "uniform mat4 ProjectionMatrix, CameraMatrix;"
        "in vec2 Position;"
        "void main(void){"
        "gl_Position = ProjectionMatrix * CameraMatrix * vec4(Position, 0.0, 1.0);"
        "}"
        );
    vertex_shader.Compile();
    fragment_shader.Source(
        "#version 140\n"
        "out vec4 fragColor;"
        "uniform vec4 DrawColor;"
        "void main(void){"
        "fragColor = DrawColor;"
        "}"
        );
    fragment_shader.Compile();
    debug_draw_program.AttachShader(vertex_shader);
    debug_draw_program.AttachShader(fragment_shader);
    debug_draw_program.Link();
}

void Box2DGLDebugDraw::DrawWorld()
{
    gl.Disable(gl::Capability::DepthTest);
    gl.Enable(oglplus::Capability::Multisample);
    debug_draw_program.Use();
    gl::Uniform<gl::Mat4f>(debug_draw_program, "ProjectionMatrix").Set(view.projection);
    gl::Uniform<gl::Mat4f>(debug_draw_program, "CameraMatrix").Set(view.camera);
    world.DrawDebugData();
}

struct Box2DGLDebugDraw::BoundPolygon
{
    gl::VertexArray polygon;
    gl::Buffer vertices_buffer;
};

gl::VertexAttribArray Box2DGLDebugDraw::BindPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color, Box2DGLDebugDraw::BoundPolygon& binding)
{
    static_assert(std::is_same<float32, GLfloat>::value, "OpenGL float and Box2D float must match");
    binding.polygon.Bind();
    binding.vertices_buffer.Bind(gl::Buffer::Target::Array);
    gl::Buffer::Data(gl::Buffer::Target::Array, vertexCount * 2, (GLfloat*)vertices, gl::BufferUsage::StreamDraw);
    auto attributes = gl::VertexAttribArray(debug_draw_program, "Position");
    attributes.Setup<gl::Vec2f>();
    attributes.Enable();
    return attributes;
}
void Box2DGLDebugDraw::DrawPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color)
{
    Box2DGLDebugDraw::BoundPolygon binding;
    auto attributes = BindPolygon(vertices, vertexCount, color, binding);
    gl::Uniform<gl::Vec4f>(debug_draw_program, "DrawColor").Set(color.r, color.g, color.b, 1.0f);
    gl.DrawArrays(gl::PrimitiveType::LineLoop, 0, vertexCount);
}

void Box2DGLDebugDraw::DrawSolidPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color)
{
    Box2DGLDebugDraw::BoundPolygon binding;
    auto attributes = BindPolygon(vertices, vertexCount, color, binding);
    gl::Uniform<gl::Vec4f>(debug_draw_program, "DrawColor").Set(color.r/2, color.g/2, color.b/2, 0.5f);
    gl.DrawArrays(gl::PrimitiveType::TriangleFan, 0, vertexCount);
    gl::Uniform<gl::Vec4f>(debug_draw_program, "DrawColor").Set(color.r, color.g, color.b, 1.0f);
    gl.DrawArrays(gl::PrimitiveType::LineLoop, 0, vertexCount);
}

struct CirclePolygon
{
    static const int vertex_count = 16;
    b2Vec2 vertices[vertex_count];
};

CirclePolygon CreateCirclePolygon(const b2Vec2& center, float32 radius)
{
    const float k_increment = 2.0f * glm::pi<float>() / CirclePolygon::vertex_count;
    float theta = 0.0f;
    CirclePolygon polygon;
    for (int32 i = 0; i < CirclePolygon::vertex_count; ++i) {
        polygon.vertices[i] = center + radius * b2Vec2(cosf(theta), sinf(theta));
        theta += k_increment;
    }
    return polygon;
}

void Box2DGLDebugDraw::DrawCircle(const b2Vec2& center, float32 radius, const b2Color& color)
{
    static_assert(std::is_same<float32, GLfloat>::value, "OpenGL float and Box2D float must match");
    DrawPolygon(CreateCirclePolygon(center, radius).vertices, CirclePolygon::vertex_count, color);
}

void Box2DGLDebugDraw::DrawSolidCircle(const b2Vec2& center, float32 radius, const b2Vec2& axis, const b2Color& color)
{
    static_assert(std::is_same<float32, GLfloat>::value, "OpenGL float and Box2D float must match");
    DrawSolidPolygon(CreateCirclePolygon(center, radius).vertices, CirclePolygon::vertex_count, color);
    DrawSegment(center, center + radius*axis, color);
}

void Box2DGLDebugDraw::DrawSegment(const b2Vec2& p1, const b2Vec2& p2, const b2Color& color)
{
    gl::VertexArray polygon;
    polygon.Bind();
    gl::Buffer vertices_buffer;
    vertices_buffer.Bind(gl::Buffer::Target::Array);
    GLfloat vertices[] = {
        p1.x, p1.y, p2.x, p2.y
    };
    gl::Buffer::Data(gl::Buffer::Target::Array, 4, (GLfloat*)vertices, gl::BufferUsage::StreamDraw);
    auto attributes = gl::VertexAttribArray(debug_draw_program, "Position");
    attributes.Setup<gl::Vec2f>();
    attributes.Enable();
    gl::Uniform<gl::Vec4f>(debug_draw_program, "DrawColor").Set(color.r, color.g, color.b, 1.0f);
    gl.DrawArrays(gl::PrimitiveType::Lines, 0, 2);
}

void Box2DGLDebugDraw::DrawTransform(const b2Transform& xf)
{
    b2Vec2 p1 = xf.p, p2;
    const float32 k_axisScale = 0.4f;

    p2 = p1 + k_axisScale * xf.q.GetXAxis();
    DrawSegment(p1, p2, b2Color(1, 0, 0));

    p2 = p1 + k_axisScale * xf.q.GetYAxis();
    DrawSegment(p1, p2, b2Color(0, 1, 0));
}

void Box2DGLDebugDraw::DrawPoint(const b2Vec2& p, float32 size, const b2Color& color)
{
    gl::VertexArray polygon;
    polygon.Bind();
    gl::Buffer vertices_buffer;
    vertices_buffer.Bind(gl::Buffer::Target::Array);
    GLfloat vertices[] = {
        p.x, p.y
    };
    gl::Buffer::Data(gl::Buffer::Target::Array, 2, (GLfloat*)vertices, gl::BufferUsage::StreamDraw);
    auto attributes = gl::VertexAttribArray(debug_draw_program, "Position");
    attributes.Setup<gl::Vec2f>();
    attributes.Enable();
    gl::Uniform<gl::Vec4f>(debug_draw_program, "DrawColor").Set(color.r, color.g, color.b, 1.0f);
    gl.PointSize(size);
    gl.DrawArrays(gl::PrimitiveType::Points, 0, 1);
    gl.PointSize(1.0f);
}

void Box2DGLDebugDraw::DrawString(int x, int y, const char *string, ...)
{
    /* Unsupported as yet. Could replace with bitmap font renderer at a later date */
}

void Box2DGLDebugDraw::DrawAABB(b2AABB* aabb, const b2Color& color)
{
    gl::VertexArray polygon;
    polygon.Bind();
    gl::Buffer vertices_buffer;
    vertices_buffer.Bind(gl::Buffer::Target::Array);
    GLfloat vertices[] = {
        aabb->lowerBound.x, aabb->lowerBound.y,
        aabb->upperBound.x, aabb->lowerBound.y,
        aabb->upperBound.x, aabb->upperBound.y,
        aabb->lowerBound.x, aabb->upperBound.y
    };
    gl::Buffer::Data(gl::Buffer::Target::Array, 8, (GLfloat*)vertices, gl::BufferUsage::StreamDraw);
    auto attributes = gl::VertexAttribArray(debug_draw_program, "Position");
    attributes.Setup<gl::Vec2f>();
    attributes.Enable();
    gl::Uniform<gl::Vec4f>(debug_draw_program, "DrawColor").Set(color.r, color.g, color.b, 1.0f);
    gl.DrawArrays(gl::PrimitiveType::LineLoop, 0, 4);
}