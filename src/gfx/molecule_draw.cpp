#include "image.h"
#include "molecule_draw.h"

#include <core/common.h>
#include <core/log.h>
#include <core/string_utils.h>
#include <gfx/gl_utils.h>
#include <mol/molecule_utils.h>

static bool is_orthographic_proj_matrix(const mat4& M) { return M[2][3] == 0.0f; }

namespace draw {
static GLuint vao = 0;
static GLuint tex[4] = {};

namespace vdw {
static GLuint program_persp = 0;
static GLuint program_ortho = 0;

static void initialize() {
    {
        GLuint v_shader = gl::compile_shader_from_file(VIAMD_SHADER_DIR "/vdw.vert", GL_VERTEX_SHADER);
        GLuint g_shader = gl::compile_shader_from_file(VIAMD_SHADER_DIR "/vdw.geom", GL_GEOMETRY_SHADER);
        GLuint f_shader = gl::compile_shader_from_file(VIAMD_SHADER_DIR "/vdw.frag", GL_FRAGMENT_SHADER);
        defer {
            glDeleteShader(v_shader);
            glDeleteShader(g_shader);
            glDeleteShader(f_shader);
        };

        if (!program_persp) program_persp = glCreateProgram();
        const GLuint shaders[] = {v_shader, g_shader, f_shader};
        gl::attach_link_detach(program_persp, shaders);
    }
    {
        GLuint v_shader = gl::compile_shader_from_file(VIAMD_SHADER_DIR "/vdw_ortho.vert", GL_VERTEX_SHADER);
        GLuint f_shader = gl::compile_shader_from_file(VIAMD_SHADER_DIR "/vdw_ortho.frag", GL_FRAGMENT_SHADER);
        defer {
            glDeleteShader(v_shader);
            glDeleteShader(f_shader);
        };

        if (!program_ortho) program_ortho = glCreateProgram();
        const GLuint shaders[] = {v_shader, f_shader};
        gl::attach_link_detach(program_ortho, shaders);
    }
}

static void shutdown() {
    if (program_persp) glDeleteProgram(program_persp);
    if (program_ortho) glDeleteProgram(program_ortho);
}

}  // namespace vdw

namespace licorice {
static GLuint program_persp = 0;
static GLuint program_ortho = 0;

static void initialize() {
    GLuint v_shader = gl::compile_shader_from_file(VIAMD_SHADER_DIR "/licorice.vert", GL_VERTEX_SHADER);
    GLuint g_shader = gl::compile_shader_from_file(VIAMD_SHADER_DIR "/licorice.geom", GL_GEOMETRY_SHADER);
    GLuint f_shader_persp = gl::compile_shader_from_file(VIAMD_SHADER_DIR "/licorice.frag", GL_FRAGMENT_SHADER);
    GLuint f_shader_ortho = gl::compile_shader_from_file(VIAMD_SHADER_DIR "/licorice.frag", GL_FRAGMENT_SHADER, "#define ORTHO");
    defer {
        glDeleteShader(v_shader);
        glDeleteShader(g_shader);
        glDeleteShader(f_shader_persp);
        glDeleteShader(f_shader_ortho);
    };

    if (!program_persp) program_persp = glCreateProgram();
    {
        const GLuint shaders[] = {v_shader, g_shader, f_shader_persp};
        gl::attach_link_detach(program_persp, shaders);
    }

    if (!program_ortho) program_ortho = glCreateProgram();
    {
        const GLuint shaders[] = {v_shader, g_shader, f_shader_ortho};
        gl::attach_link_detach(program_ortho, shaders);
    }
}

static void shutdown() {
    if (program_persp) glDeleteProgram(program_persp);
    if (program_ortho) glDeleteProgram(program_ortho);
}

}  // namespace licorice

namespace ribbon {
static GLuint program = 0;

void intitialize() {
    GLuint v_shader = gl::compile_shader_from_file(VIAMD_SHADER_DIR "/ribbons.vert", GL_VERTEX_SHADER);
    GLuint g_shader = gl::compile_shader_from_file(VIAMD_SHADER_DIR "/ribbons.geom", GL_GEOMETRY_SHADER);
    GLuint f_shader = gl::compile_shader_from_file(VIAMD_SHADER_DIR "/ribbons.frag", GL_FRAGMENT_SHADER);
    defer {
        glDeleteShader(v_shader);
        glDeleteShader(g_shader);
        glDeleteShader(f_shader);
    };

    if (!program) program = glCreateProgram();
    const GLuint shaders[] = {v_shader, g_shader, f_shader};
    gl::attach_link_detach(program, shaders);
}

void shutdown() {
    if (program) glDeleteProgram(program);
}

}  // namespace ribbon

namespace cartoon {
static GLuint program = 0;

void intitialize() {
    GLuint v_shader = gl::compile_shader_from_file(VIAMD_SHADER_DIR "/cartoon.vert", GL_VERTEX_SHADER);
    GLuint g_shader = gl::compile_shader_from_file(VIAMD_SHADER_DIR "/cartoon.geom", GL_GEOMETRY_SHADER);
    GLuint f_shader = gl::compile_shader_from_file(VIAMD_SHADER_DIR "/cartoon.frag", GL_FRAGMENT_SHADER);
    defer {
        glDeleteShader(v_shader);
        glDeleteShader(g_shader);
        glDeleteShader(f_shader);
    };

    if (!program) program = glCreateProgram();
    const GLuint shaders[] = {v_shader, g_shader, f_shader};
    gl::attach_link_detach(program, shaders);
}

void shutdown() {
    if (program) glDeleteProgram(program);
}

}  // namespace cartoon

namespace backbone_spline {
static GLuint extract_control_points_program = 0;
static GLuint compute_spline_program = 0;
static GLuint draw_spline_program = 0;

void initialize() {
    {
        GLuint v_shader = gl::compile_shader_from_file(VIAMD_SHADER_DIR "/backbone_control_points.vert", GL_VERTEX_SHADER);
        GLuint g_shader = gl::compile_shader_from_file(VIAMD_SHADER_DIR "/backbone_control_points.geom", GL_GEOMETRY_SHADER);
        defer {
            glDeleteShader(v_shader);
            glDeleteShader(g_shader);
        };

        const GLuint shaders[] = {v_shader, g_shader};
        const GLchar* feedback_varyings[] = {"out_control_point",     "out_support_vector_xy", "out_support_vector_z_tangent_vector_x",
                                             "out_tangent_vector_yz", "out_classification",    "out_atom_index"};
        if (!extract_control_points_program) extract_control_points_program = glCreateProgram();
        gl::attach_link_detach_with_transform_feedback(extract_control_points_program, shaders, feedback_varyings, GL_INTERLEAVED_ATTRIBS);
    }

    {
        GLuint v_shader = gl::compile_shader_from_file(VIAMD_SHADER_DIR "/backbone_compute_spline.vert", GL_VERTEX_SHADER);
        GLuint g_shader = gl::compile_shader_from_file(VIAMD_SHADER_DIR "/backbone_compute_spline.geom", GL_GEOMETRY_SHADER);
        defer {
            glDeleteShader(v_shader);
            glDeleteShader(g_shader);
        };

        const GLuint shaders[] = {v_shader, g_shader};
        const GLchar* feedback_varyings[] = {"out_control_point",     "out_support_vector_xy", "out_support_vector_z_tangent_vector_x",
                                             "out_tangent_vector_yz", "out_classification",    "out_atom_index"};
        if (!compute_spline_program) compute_spline_program = glCreateProgram();
        gl::attach_link_detach_with_transform_feedback(compute_spline_program, shaders, feedback_varyings, GL_INTERLEAVED_ATTRIBS);
    }

    {
        GLuint v_shader = gl::compile_shader_from_file(VIAMD_SHADER_DIR "/backbone_draw_spline.vert", GL_VERTEX_SHADER);
        GLuint g_shader = gl::compile_shader_from_file(VIAMD_SHADER_DIR "/backbone_draw_spline.geom", GL_GEOMETRY_SHADER);
        GLuint f_shader = gl::compile_shader_from_file(VIAMD_SHADER_DIR "/backbone_draw_spline.frag", GL_FRAGMENT_SHADER);
        defer {
            glDeleteShader(v_shader);
            glDeleteShader(g_shader);
            glDeleteShader(f_shader);
        };

        const GLuint shaders[] = {v_shader, g_shader, f_shader};
        if (!draw_spline_program) draw_spline_program = glCreateProgram();
        gl::attach_link_detach(draw_spline_program, shaders);
    }
}

void shutdown() {
    if (extract_control_points_program) glDeleteProgram(extract_control_points_program);
    if (compute_spline_program) glDeleteProgram(compute_spline_program);
    if (draw_spline_program) glDeleteProgram(draw_spline_program);
}
}  // namespace backbone_spline

namespace pbc_view_velocity {
static GLuint program = 0;
void initialize() {
    GLuint v_shader = gl::compile_shader_from_file(VIAMD_SHADER_DIR "/compute_pbc_view_velocity.vert", GL_VERTEX_SHADER);
    defer { glDeleteShader(v_shader); };

    const GLuint shaders[] = {v_shader};
    const GLchar* feedback_varyings[] = {"out_view_velocity"};
    if (!program) program = glCreateProgram();
    gl::attach_link_detach_with_transform_feedback(program, shaders, feedback_varyings, GL_INTERLEAVED_ATTRIBS);
}

void shutdown() {
    if (program) glDeleteProgram(program);
}
}  // namespace pbc_view_velocity

namespace lean_and_mean {
namespace vdw {
static GLuint program = 0;

static void initialize() {
    GLuint v_shader = gl::compile_shader_from_file(VIAMD_SHADER_DIR "/lean_and_mean/vdw.vert", GL_VERTEX_SHADER);
    GLuint g_shader = gl::compile_shader_from_file(VIAMD_SHADER_DIR "/lean_and_mean/vdw.geom", GL_GEOMETRY_SHADER);
    GLuint f_shader = gl::compile_shader_from_file(VIAMD_SHADER_DIR "/lean_and_mean/vdw.frag", GL_FRAGMENT_SHADER);
    defer {
        glDeleteShader(v_shader);
        glDeleteShader(g_shader);
        glDeleteShader(f_shader);
    };

    if (!program) program = glCreateProgram();
    const GLuint shaders[] = {v_shader, g_shader, f_shader};
    gl::attach_link_detach(program, shaders);
}

static void shutdown() {
    if (program) glDeleteProgram(program);
}
}  // namespace vdw

namespace licorice {
static GLuint program = 0;

static void initialize() {
    GLuint v_shader = gl::compile_shader_from_file(VIAMD_SHADER_DIR "/lean_and_mean/licorice.vert", GL_VERTEX_SHADER);
    GLuint g_shader = gl::compile_shader_from_file(VIAMD_SHADER_DIR "/lean_and_mean/licorice.geom", GL_GEOMETRY_SHADER);
    GLuint f_shader = gl::compile_shader_from_file(VIAMD_SHADER_DIR "/lean_and_mean/licorice.frag", GL_FRAGMENT_SHADER);
    defer {
        glDeleteShader(v_shader);
        glDeleteShader(g_shader);
        glDeleteShader(f_shader);
    };

    if (!program) program = glCreateProgram();
    const GLuint shaders[] = {v_shader, g_shader, f_shader};
    gl::attach_link_detach(program, shaders);
}

static void shutdown() {
    if (program) glDeleteProgram(program);
}

}  // namespace licorice

namespace ribbon {
static GLuint program = 0;

void intitialize() {
    GLuint v_shader = gl::compile_shader_from_file(VIAMD_SHADER_DIR "/lean_and_mean/ribbons.vert", GL_VERTEX_SHADER);
    GLuint g_shader = gl::compile_shader_from_file(VIAMD_SHADER_DIR "/lean_and_mean/ribbons.geom", GL_GEOMETRY_SHADER);
    GLuint f_shader = gl::compile_shader_from_file(VIAMD_SHADER_DIR "/lean_and_mean/ribbons.frag", GL_FRAGMENT_SHADER);
    defer {
        glDeleteShader(v_shader);
        glDeleteShader(g_shader);
        glDeleteShader(f_shader);
    };

    if (!program) program = glCreateProgram();
    const GLuint shaders[] = {v_shader, g_shader, f_shader};
    gl::attach_link_detach(program, shaders);
}

void shutdown() {
    if (program) glDeleteProgram(program);
}

}  // namespace ribbon
}  // namespace lean_and_mean

void initialize() {
    if (!vao) glGenVertexArrays(1, &vao);
    if (!tex[0]) glGenTextures(4, tex);
    vdw::initialize();
    licorice::initialize();
    ribbon::intitialize();
    cartoon::intitialize();
    backbone_spline::initialize();
    lean_and_mean::vdw::initialize();
    lean_and_mean::licorice::initialize();
    lean_and_mean::ribbon::intitialize();
    pbc_view_velocity::initialize();
}

void shutdown() {
    if (vao) glDeleteVertexArrays(1, &vao);
    if (tex[0]) glDeleteTextures(4, tex);
    vdw::shutdown();
    licorice::shutdown();
    ribbon::shutdown();
    cartoon::shutdown();
    backbone_spline::shutdown();
    lean_and_mean::vdw::shutdown();
    lean_and_mean::licorice::shutdown();
    lean_and_mean::ribbon::shutdown();
    pbc_view_velocity::shutdown();
}

void draw_vdw(GLuint atom_position_buffer, GLuint atom_radius_buffer, GLuint atom_color_buffer, GLuint atom_view_velocity_buffer, int32 atom_count,
              const ViewParam& view_param, float radius_scale) {
    ASSERT(glIsBuffer(atom_position_buffer));
    ASSERT(glIsBuffer(atom_radius_buffer));
    ASSERT(glIsBuffer(atom_color_buffer));
    ASSERT(glIsBuffer(atom_view_velocity_buffer));

    const mat4 curr_view_to_prev_clip_mat = view_param.matrix.previous.view_proj_jittered * view_param.matrix.inverse.view;
    const vec2 res = view_param.resolution;
    const vec4 jitter_uv = vec4(view_param.jitter.current / res, view_param.jitter.previous / res);
    static uint32 frame = 0;
    frame++;

    const bool ortho = is_orthographic_proj_matrix(view_param.matrix.current.proj);
    GLuint prog = ortho ? vdw::program_ortho : vdw::program_persp;
    glUseProgram(prog);

    if (ortho) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_BUFFER, tex[0]);
        glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, atom_position_buffer);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_BUFFER, tex[1]);
        glTexBuffer(GL_TEXTURE_BUFFER, GL_R32F, atom_radius_buffer);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_BUFFER, tex[2]);
        glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA8, atom_color_buffer);

        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_BUFFER, tex[3]);
        glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, atom_view_velocity_buffer);

        glBindBuffer(GL_TEXTURE_BUFFER, 0);
        glActiveTexture(GL_TEXTURE0);

        // Uniforms
        glUniform1i(glGetUniformLocation(prog, "u_buf_position"), 0);
        glUniform1i(glGetUniformLocation(prog, "u_buf_radius"), 1);
        glUniform1i(glGetUniformLocation(prog, "u_buf_color"), 2);
        glUniform1i(glGetUniformLocation(prog, "u_buf_view_velocity"), 3);
        glUniformMatrix4fv(glGetUniformLocation(prog, "u_view_mat"), 1, GL_FALSE, &view_param.matrix.current.view[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(prog, "u_proj_mat"), 1, GL_FALSE, &view_param.matrix.current.proj_jittered[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(prog, "u_curr_view_to_prev_clip_mat"), 1, GL_FALSE, &curr_view_to_prev_clip_mat[0][0]);
        glUniform1f(glGetUniformLocation(prog, "u_radius_scale"), radius_scale);
        glUniform4fv(glGetUniformLocation(prog, "u_jitter_uv"), 1, &jitter_uv[0]);

        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, atom_count * 3);
        glBindVertexArray(0);
    } else {
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, atom_position_buffer);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(AtomPosition), (const GLvoid*)0);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, atom_radius_buffer);
        glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(AtomRadius), (const GLvoid*)0);
        glEnableVertexAttribArray(1);

        glBindBuffer(GL_ARRAY_BUFFER, atom_color_buffer);
        glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(AtomColor), (const GLvoid*)0);
        glEnableVertexAttribArray(2);

        glBindBuffer(GL_ARRAY_BUFFER, atom_view_velocity_buffer);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(AtomVelocity), (const GLvoid*)0);
        glEnableVertexAttribArray(3);

        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // Uniforms
        glUniformMatrix4fv(glGetUniformLocation(prog, "u_view_mat"), 1, GL_FALSE, &view_param.matrix.current.view[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(prog, "u_proj_mat"), 1, GL_FALSE, &view_param.matrix.current.proj_jittered[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(prog, "u_inv_proj_mat"), 1, GL_FALSE, &view_param.matrix.inverse.proj_jittered[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(prog, "u_curr_view_to_prev_clip_mat"), 1, GL_FALSE, &curr_view_to_prev_clip_mat[0][0]);
        glUniform1f(glGetUniformLocation(prog, "u_radius_scale"), radius_scale);
        glUniform4fv(glGetUniformLocation(prog, "u_jitter_uv"), 1, &jitter_uv[0]);
        glUniform1ui(glGetUniformLocation(prog, "u_frame"), frame);

        glDrawArrays(GL_POINTS, 0, atom_count);
        glBindVertexArray(0);
    }
    glUseProgram(0);
}

void draw_licorice(GLuint atom_position_buffer, GLuint atom_color_buffer, GLuint atom_velocity_buffer, GLuint bond_buffer, int32 bond_count,
                   const ViewParam& view_param, float radius_scale) {
    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, atom_position_buffer);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(AtomPosition), (const GLvoid*)0);

    glBindBuffer(GL_ARRAY_BUFFER, atom_color_buffer);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(AtomColor), (const GLvoid*)0);

    glBindBuffer(GL_ARRAY_BUFFER, atom_velocity_buffer);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(AtomVelocity), (const GLvoid*)0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bond_buffer);

    const mat4 curr_view_to_prev_clip_mat = view_param.matrix.previous.view_proj_jittered * view_param.matrix.inverse.view;
    const vec2 res = view_param.resolution;
    const vec4 jitter_uv = vec4(view_param.jitter.current / res, view_param.jitter.previous / res);

    const bool ortho = is_orthographic_proj_matrix(view_param.matrix.current.proj_jittered);
    const GLuint prog = ortho ? licorice::program_ortho : licorice::program_persp;
    glUseProgram(prog);

    glUniformMatrix4fv(glGetUniformLocation(prog, "u_view_mat"), 1, GL_FALSE, &view_param.matrix.current.view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(prog, "u_proj_mat"), 1, GL_FALSE, &view_param.matrix.current.proj_jittered[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(prog, "u_curr_view_to_prev_clip_mat"), 1, GL_FALSE, &curr_view_to_prev_clip_mat[0][0]);
    glUniform1f(glGetUniformLocation(prog, "u_radius"), 0.25f * radius_scale);
    glUniform4fv(glGetUniformLocation(prog, "u_jitter_uv"), 1, &jitter_uv[0]);

    glDrawElements(GL_LINES, bond_count * 2, GL_UNSIGNED_INT, 0);
    glUseProgram(0);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void compute_backbone_control_points(GLuint dst_buffer, GLuint atom_position_buffer, GLuint backbone_index_buffer, int num_backbone_indices,
                                     GLuint ramachandran_tex) {
    glEnable(GL_RASTERIZER_DISCARD);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, atom_position_buffer);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(AtomPosition), (const GLvoid*)0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, backbone_index_buffer);

    glUseProgram(backbone_spline::extract_control_points_program);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, ramachandran_tex);
    //@TODO: Replace ramachandran texture to some vertex attribute which holds the type of secondary structure

    glUniform1i(glGetUniformLocation(backbone_spline::extract_control_points_program, "u_ramachandran_tex"), 0);

    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, dst_buffer);
    glBeginTransformFeedback(GL_POINTS);
    glDrawElements(GL_TRIANGLES_ADJACENCY, num_backbone_indices, GL_UNSIGNED_INT, 0);
    glEndTransformFeedback();

    glUseProgram(0);

    glDisable(GL_RASTERIZER_DISCARD);
}

void compute_backbone_spline(GLuint dst_buffer, GLuint control_point_buffer, GLuint control_point_index_buffer, int num_control_point_indices,
                             float tension) {
    glEnable(GL_RASTERIZER_DISCARD);
    glEnable(GL_PRIMITIVE_RESTART);
    glPrimitiveRestartIndex(0xFFFFFFFFU);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, control_point_buffer);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ControlPoint), (const GLvoid*)offsetof(ControlPoint, position));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_SHORT, GL_TRUE, sizeof(ControlPoint), (const GLvoid*)offsetof(ControlPoint, support_vector));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ControlPoint), (const GLvoid*)offsetof(ControlPoint, classification));
    glEnableVertexAttribArray(3);
    glVertexAttribIPointer(3, 1, GL_UNSIGNED_INT, sizeof(ControlPoint), (const GLvoid*)offsetof(ControlPoint, atom_index));

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, control_point_index_buffer);

    glUseProgram(backbone_spline::compute_spline_program);

    glUniform1f(glGetUniformLocation(backbone_spline::compute_spline_program, "u_tension"), tension);

    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, dst_buffer);
    glBeginTransformFeedback(GL_POINTS);
    glDrawElements(GL_LINE_STRIP_ADJACENCY, num_control_point_indices, GL_UNSIGNED_INT, 0);
    glEndTransformFeedback();

    glUseProgram(0);

    glDisable(GL_PRIMITIVE_RESTART);
    glDisable(GL_RASTERIZER_DISCARD);
}

void compute_pbc_view_velocity(GLuint dst_buffer, GLuint position_buffer, GLuint old_position_buffer, int count, const ViewParam& view_param,
                               const vec3& box_ext) {
    glEnable(GL_RASTERIZER_DISCARD);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, position_buffer);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(AtomPosition), (const GLvoid*)0);

    glBindBuffer(GL_ARRAY_BUFFER, old_position_buffer);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(AtomPosition), (const GLvoid*)0);

    glUseProgram(pbc_view_velocity::program);

    glUniformMatrix4fv(glGetUniformLocation(pbc_view_velocity::program, "u_view_mat"), 1, GL_FALSE, &view_param.matrix.current.view[0][0]);
    glUniform3fv(glGetUniformLocation(pbc_view_velocity::program, "u_box_ext"), 1, &box_ext[0]);

    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, dst_buffer);
    glBeginTransformFeedback(GL_POINTS);
    glDrawArrays(GL_POINTS, 0, count);
    glEndTransformFeedback();

    glUseProgram(0);

    glDisable(GL_RASTERIZER_DISCARD);
}

void draw_spline(GLuint spline_buffer, GLuint spline_index_buffer, int32 num_spline_indices, const ViewParam& view_param, uint32 s_color,
                 uint32 v_color, uint32 t_color) {
    glEnable(GL_PRIMITIVE_RESTART);
    glPrimitiveRestartIndex(0xFFFFFFFFU);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, spline_buffer);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ControlPoint), (const GLvoid*)offsetof(ControlPoint, position));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_SHORT, GL_TRUE, sizeof(ControlPoint), (const GLvoid*)offsetof(ControlPoint, support_vector));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_SHORT, GL_TRUE, sizeof(ControlPoint), (const GLvoid*)offsetof(ControlPoint, tangent_vector));

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, spline_index_buffer);

    GLuint prog = backbone_spline::draw_spline_program;
    glUseProgram(prog);
    glUniformMatrix4fv(glGetUniformLocation(prog, "u_view_proj_mat"), 1, GL_FALSE, &(view_param.matrix.current.view_proj_jittered)[0][0]);
    glUniform4fv(glGetUniformLocation(prog, "u_s_color"), 1, &math::convert_color(s_color)[0]);
    glUniform4fv(glGetUniformLocation(prog, "u_v_color"), 1, &math::convert_color(v_color)[0]);
    glUniform4fv(glGetUniformLocation(prog, "u_t_color"), 1, &math::convert_color(t_color)[0]);

    glDrawElements(GL_LINE_STRIP, num_spline_indices, GL_UNSIGNED_INT, 0);
    glUseProgram(0);

    glDisable(GL_PRIMITIVE_RESTART);
}

void draw_ribbons(GLuint spline_buffer, GLuint spline_index_buffer, GLuint atom_color_buffer, GLuint atom_velocity_buffer, int32 num_spline_indices,
                  const ViewParam& view_param) {
    glEnable(GL_PRIMITIVE_RESTART);
    glPrimitiveRestartIndex(0xFFFFFFFFU);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, spline_buffer);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ControlPoint), (const GLvoid*)offsetof(ControlPoint, position));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_SHORT, GL_TRUE, sizeof(ControlPoint), (const GLvoid*)offsetof(ControlPoint, support_vector));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_SHORT, GL_TRUE, sizeof(ControlPoint), (const GLvoid*)offsetof(ControlPoint, tangent_vector));
    glEnableVertexAttribArray(3);
    glVertexAttribIPointer(3, 1, GL_UNSIGNED_INT, sizeof(ControlPoint), (const GLvoid*)offsetof(ControlPoint, atom_index));

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, spline_index_buffer);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_BUFFER, tex[0]);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA8, atom_color_buffer);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_BUFFER, tex[1]);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, atom_velocity_buffer);

    const vec2 res = view_param.resolution;
    const vec4 jitter_uv = vec4(view_param.jitter.current / res, view_param.jitter.previous / res);

    glUseProgram(ribbon::program);

    // Uniforms
    glUniform1i(glGetUniformLocation(ribbon::program, "u_atom_color_buffer"), 0);
    glUniform1i(glGetUniformLocation(ribbon::program, "u_atom_velocity_buffer"), 1);
    glUniformMatrix4fv(glGetUniformLocation(ribbon::program, "u_normal_mat"), 1, GL_FALSE, &view_param.matrix.current.norm[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(ribbon::program, "u_view_proj_mat"), 1, GL_FALSE, &view_param.matrix.current.view_proj_jittered[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(ribbon::program, "u_prev_view_proj_mat"), 1, GL_FALSE,
                       &view_param.matrix.previous.view_proj_jittered[0][0]);
    glUniform4fv(glGetUniformLocation(ribbon::program, "u_jitter_uv"), 1, &jitter_uv[0]);

    glDrawElements(GL_LINE_STRIP, num_spline_indices, GL_UNSIGNED_INT, 0);
    glUseProgram(0);

    glDisable(GL_PRIMITIVE_RESTART);
}

void draw_cartoon(GLuint spline_buffer, GLuint spline_index_buffer, GLuint atom_color_buffer, int32 num_spline_indices, const ViewParam& view_param) {
    glEnable(GL_PRIMITIVE_RESTART);
    glPrimitiveRestartIndex(0xFFFFFFFFU);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, spline_buffer);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ControlPoint), (const GLvoid*)offsetof(ControlPoint, position));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_SHORT, GL_TRUE, sizeof(ControlPoint), (const GLvoid*)offsetof(ControlPoint, support_vector));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_SHORT, GL_TRUE, sizeof(ControlPoint), (const GLvoid*)offsetof(ControlPoint, tangent_vector));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ControlPoint), (const GLvoid*)offsetof(ControlPoint, classification));
    glEnableVertexAttribArray(4);
    glVertexAttribIPointer(4, 1, GL_UNSIGNED_INT, sizeof(ControlPoint), (const GLvoid*)offsetof(ControlPoint, atom_index));

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, spline_index_buffer);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_BUFFER, tex[0]);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA8, atom_color_buffer);

    GLuint prog = cartoon::program;
    glUseProgram(prog);
    glUniform1i(glGetUniformLocation(prog, "u_atom_color_buffer"), 0);
    glUniformMatrix4fv(glGetUniformLocation(prog, "u_normal_mat"), 1, GL_FALSE, &view_param.matrix.current.norm[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(prog, "u_view_proj_mat"), 1, GL_FALSE, &view_param.matrix.current.view_proj_jittered[0][0]);
    glDrawElements(GL_LINE_STRIP, num_spline_indices, GL_UNSIGNED_INT, 0);
    glUseProgram(0);

    glDisable(GL_PRIMITIVE_RESTART);
}

namespace lean_and_mean {

void draw_vdw(GLuint atom_position_buffer, GLuint atom_radius_buffer, GLuint atom_color_buffer, GLuint atom_mask_buffer, int32 atom_count,
              const ViewParam& view_param, float radius_scale, vec4 color, uint32 mask) {
    ASSERT(glIsBuffer(atom_position_buffer));
    ASSERT(glIsBuffer(atom_radius_buffer));
    ASSERT(glIsBuffer(atom_color_buffer));
    ASSERT(glIsBuffer(atom_mask_buffer));

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, atom_position_buffer);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(AtomPosition), (const GLvoid*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, atom_radius_buffer);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(AtomRadius), (const GLvoid*)0);
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, atom_color_buffer);
    glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(AtomColor), (const GLvoid*)0);
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, atom_mask_buffer);
    glVertexAttribIPointer(3, 1, GL_UNSIGNED_BYTE, sizeof(AtomMask), (const GLvoid*)0);
    glEnableVertexAttribArray(3);

    const vec2 res = view_param.resolution;
    const vec4 jitter_uv = vec4(view_param.jitter.current / res, view_param.jitter.previous / res);

    GLuint prog = vdw::program;
    glUseProgram(prog);

    // Uniforms
    glUniformMatrix4fv(glGetUniformLocation(prog, "u_view_mat"), 1, GL_FALSE, &view_param.matrix.current.view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(prog, "u_proj_mat"), 1, GL_FALSE, &view_param.matrix.current.proj_jittered[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(prog, "u_inv_proj_mat"), 1, GL_FALSE, &view_param.matrix.inverse.proj_jittered[0][0]);
    glUniform4fv(glGetUniformLocation(prog, "u_jitter_uv"), 1, &jitter_uv[0]);
    glUniform4fv(glGetUniformLocation(prog, "u_color"), 1, &color[0]);
    glUniform1f(glGetUniformLocation(prog, "u_radius_scale"), radius_scale);
    glUniform1ui(glGetUniformLocation(prog, "u_mask"), mask);

    glDrawArrays(GL_POINTS, 0, atom_count);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glUseProgram(0);
    glBindVertexArray(0);
}

void draw_licorice(GLuint atom_position_buffer, GLuint atom_color_buffer, GLuint atom_mask_buffer, GLuint bond_buffer, int32 bond_count,
                   const ViewParam& view_param, float radius_scale, vec4 color, uint32 mask) {
    ASSERT(glIsBuffer(atom_position_buffer));
    ASSERT(glIsBuffer(atom_color_buffer));
    ASSERT(glIsBuffer(atom_mask_buffer));
    ASSERT(glIsBuffer(bond_buffer));

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, atom_position_buffer);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(AtomPosition), (const GLvoid*)0);

    glBindBuffer(GL_ARRAY_BUFFER, atom_color_buffer);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(AtomColor), (const GLvoid*)0);

    glBindBuffer(GL_ARRAY_BUFFER, atom_mask_buffer);
    glEnableVertexAttribArray(2);
    glVertexAttribIPointer(2, 1, GL_UNSIGNED_BYTE, sizeof(AtomMask), (const GLvoid*)0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bond_buffer);

    GLuint prog = licorice::program;
    glUseProgram(prog);

    glUniformMatrix4fv(glGetUniformLocation(prog, "u_view_mat"), 1, GL_FALSE, &view_param.matrix.current.view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(prog, "u_proj_mat"), 1, GL_FALSE, &view_param.matrix.current.proj_jittered[0][0]);
    glUniform4fv(glGetUniformLocation(prog, "u_color"), 1, &color[0]);
    glUniform1ui(glGetUniformLocation(prog, "u_mask"), mask);
    glUniform1f(glGetUniformLocation(prog, "u_radius"), 0.25f * radius_scale);

    glDrawElements(GL_LINES, bond_count * 2, GL_UNSIGNED_INT, 0);
    glUseProgram(0);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void draw_ribbons(GLuint spline_buffer, GLuint spline_index_buffer, GLuint atom_color_buffer, GLuint atom_mask_buffer, int32 num_spline_indices,
                  const ViewParam& view_param, float scale, vec4 color, uint32 mask) {
    ASSERT(glIsBuffer(spline_buffer));
    ASSERT(glIsBuffer(spline_index_buffer));
    ASSERT(glIsBuffer(atom_color_buffer));
    ASSERT(glIsBuffer(atom_mask_buffer));

    glEnable(GL_PRIMITIVE_RESTART);
    glPrimitiveRestartIndex(0xFFFFFFFFU);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, spline_buffer);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ControlPoint), (const GLvoid*)offsetof(ControlPoint, position));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_SHORT, GL_TRUE, sizeof(ControlPoint), (const GLvoid*)offsetof(ControlPoint, support_vector));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_SHORT, GL_TRUE, sizeof(ControlPoint), (const GLvoid*)offsetof(ControlPoint, tangent_vector));
    glEnableVertexAttribArray(3);
    glVertexAttribIPointer(3, 1, GL_UNSIGNED_INT, sizeof(ControlPoint), (const GLvoid*)offsetof(ControlPoint, atom_index));

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, spline_index_buffer);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_BUFFER, tex[0]);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA8, atom_color_buffer);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_BUFFER, tex[1]);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_R8UI, atom_mask_buffer);

    const vec2 ribbon_scale = vec2(1.0f, 0.1f) * scale;

    GLuint prog = ribbon::program;
    glUseProgram(prog);

    // Uniforms
    glUniform1i(glGetUniformLocation(prog, "u_atom_color_buffer"), 0);
    glUniform1i(glGetUniformLocation(prog, "u_atom_mask_buffer"), 1);
    glUniformMatrix4fv(glGetUniformLocation(prog, "u_view_proj_mat"), 1, GL_FALSE, &view_param.matrix.current.view_proj_jittered[0][0]);
    glUniform4fv(glGetUniformLocation(prog, "u_color"), 1, &color[0]);
    glUniform2fv(glGetUniformLocation(prog, "u_scale"), 1, &ribbon_scale[0]);
    glUniform1ui(glGetUniformLocation(prog, "u_mask"), mask);

    glDrawElements(GL_LINE_STRIP, num_spline_indices, GL_UNSIGNED_INT, 0);
    glUseProgram(0);

    glDisable(GL_PRIMITIVE_RESTART);
}

}  // namespace lean_and_mean
}  // namespace draw
