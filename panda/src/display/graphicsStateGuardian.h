// Filename: graphicsStateGuardian.h
// Created by:  drose (02Feb99)
//
////////////////////////////////////////////////////////////////////
//
// PANDA 3D SOFTWARE
// Copyright (c) 2001, Disney Enterprises, Inc.  All rights reserved
//
// All use of this software is subject to the terms of the Panda 3d
// Software license.  You should have received a copy of this license
// along with this source code; you will also find a current copy of
// the license at http://www.panda3d.org/license.txt .
//
// To contact the maintainers of this program write to
// panda3d@yahoogroups.com .
//
////////////////////////////////////////////////////////////////////

#ifndef GRAPHICSSTATEGUARDIAN_H
#define GRAPHICSSTATEGUARDIAN_H

#include "pandabase.h"

#include "savedFrameBuffer.h"
#include "frameBufferStack.h"
#include "displayRegionStack.h"
#include "lensStack.h"

#include "graphicsStateGuardianBase.h"
#include "nodeTransition.h"
#include "nodeTransitionCache.h"
#include "luse.h"
#include "coordinateSystem.h"
#include "factory.h"
#include "renderTraverser.h"
#include "pStatCollector.h"
#include "allTransitionsWrapper.h"
#include "transformState.h"
#include "renderState.h"

#include "notify.h"
#include "pvector.h"

class AllTransitionsWrapper;

////////////////////////////////////////////////////////////////////
//       Class : GraphicsStateGuardian
// Description : Encapsulates all the communication with a particular
//               instance of a given rendering backend.  Tries to
//               guarantee that redundant state-change requests are
//               not issued (hence "state guardian").
//
//               There will be one of these objects for each different
//               graphics context active in the system.
////////////////////////////////////////////////////////////////////
class EXPCL_PANDA GraphicsStateGuardian : public GraphicsStateGuardianBase {
  //
  // Interfaces all GSGs should have
  //
public:
  GraphicsStateGuardian(GraphicsWindow *win);
  virtual ~GraphicsStateGuardian();

PUBLISHED:
  INLINE void set_render_traverser(RenderTraverser *rt);
  INLINE RenderTraverser *get_render_traverser() const;

  virtual void set_color_clear_value(const Colorf& value);
  virtual void set_depth_clear_value(const float value);
  virtual void set_stencil_clear_value(const bool value);
  virtual void set_accum_clear_value(const Colorf& value);
  INLINE Colorf get_color_clear_value(void) const {
    return _color_clear_value;
  }
  INLINE float get_depth_clear_value(void) const {
    return _depth_clear_value;
  }
  INLINE bool get_stencil_clear_value(void) const {
    return _stencil_clear_value;
  }
  INLINE Colorf get_accum_clear_value(void) const {
    return _accum_clear_value;
  }

  void enable_frame_clear(bool clear_color, bool clear_depth);
  void release_all_textures();
  void release_all_geoms();

  void clear_attribute(TypeHandle type);
  NodeTransition *get_attribute(TypeHandle type) const;

public:
  INLINE bool is_closed() const;

  virtual TextureContext *prepare_texture(Texture *tex);
  virtual void apply_texture(TextureContext *tc);
  virtual void release_texture(TextureContext *tc);

  virtual GeomNodeContext *prepare_geom_node(GeomNode *node);
  virtual void draw_geom_node(GeomNode *node, GeomNodeContext *gnc);
  virtual void release_geom_node(GeomNodeContext *gnc);

  virtual GeomContext *prepare_geom(Geom *geom);
  virtual void release_geom(GeomContext *gc);

  virtual void clear(const RenderBuffer &buffer)=0;
  virtual void clear(const RenderBuffer &buffer, const DisplayRegion* region)=0;
  virtual void clear_framebuffer();

  virtual void prepare_display_region()=0;
  virtual bool prepare_lens();

  virtual void render_frame()=0;
  virtual void render_scene(Node *root, LensNode *projnode)=0;
  virtual void render_subgraph(RenderTraverser *traverser,
                               Node *subgraph, LensNode *projnode,
                               const AllTransitionsWrapper &net_trans)=0;
  virtual void render_subgraph(RenderTraverser *traverser,
                               Node *subgraph,
                               const AllTransitionsWrapper &net_trans)=0;

  INLINE void enable_normals(bool val) { _normals_enabled = val; }


  // These functions will be queried by the GeomIssuer to determine if
  // it should issue normals, texcoords, and/or colors, based on the
  // GSG's current state.
  virtual bool wants_normals() const;
  virtual bool wants_texcoords() const;
  virtual bool wants_colors() const;

  virtual void begin_decal(GeomNode *base_geom, AllTransitionsWrapper &attrib);
  virtual void end_decal(GeomNode *base_geom);

  virtual void reset();

  // *** QP
  void modify_state(const NodeTransitions &new_state);
  //  void modify_state(const NodeTransitionCache &new_state);
  void set_state(const NodeTransitionCache &new_state);
  INLINE void set_state(const AllTransitionsWrapper &new_state);
  //  INLINE const NodeTransitionCache *get_state() const;

  INLINE void modify_state(const RenderState *state);
  INLINE void set_state(const RenderState *state);
  INLINE void set_transform(const TransformState *transform);

  RenderBuffer get_render_buffer(int buffer_type);

  INLINE LensNode *get_current_camera(void) const ;
  INLINE const Node* get_current_root_node(void) const;

  INLINE const DisplayRegion *get_current_display_region(void) const;
  INLINE const Lens *get_current_lens() const;

  INLINE DisplayRegionStack push_display_region(const DisplayRegion *dr);
  INLINE void pop_display_region(DisplayRegionStack &node);
  INLINE FrameBufferStack push_frame_buffer(const RenderBuffer &buffer,
                                            const DisplayRegion *dr);
  INLINE void pop_frame_buffer(FrameBufferStack &node);

  INLINE LensStack push_lens(const Lens *lens);
  INLINE void pop_lens(LensStack &stack);
  INLINE bool set_lens(const Lens *lens);

  INLINE void set_coordinate_system(CoordinateSystem cs);
  INLINE CoordinateSystem get_coordinate_system() const;
  virtual CoordinateSystem get_internal_coordinate_system() const;

  // This function may only be called during a render traversal; it
  // will compute the distance to the indicated point, assumed to be
  // in modelview coordinates, from the camera plane.  This is a
  // virtual function because different GSG's may define the modelview
  // coordinate space differently.
  virtual float compute_distance_to(const LPoint3f &point) const=0;

  INLINE void clear_cached_state(void) { _state.clear(); };  

  virtual void issue_transform(const TransformState *transform);
  virtual void issue_color(const ColorAttrib *attrib);

protected:
  virtual PT(SavedFrameBuffer) save_frame_buffer(const RenderBuffer &buffer,
                                                 CPT(DisplayRegion) dr)=0;
  virtual void restore_frame_buffer(SavedFrameBuffer *frame_buffer)=0;

  bool mark_prepared_texture(TextureContext *tc);
  bool unmark_prepared_texture(TextureContext *tc);
  bool mark_prepared_geom(GeomContext *gc);
  bool unmark_prepared_geom(GeomContext *gc);
  bool mark_prepared_geom_node(GeomNodeContext *gnc);
  bool unmark_prepared_geom_node(GeomNodeContext *gnc);

  virtual void close_gsg();

#ifdef DO_PSTATS
  // These functions are used to update the active texture memory
  // usage record (and other frame-based measurements) in Pstats.
  void init_frame_pstats();
  void add_to_texture_record(TextureContext *tc);
  void add_to_geom_record(GeomContext *gc);
  void add_to_geom_node_record(GeomNodeContext *gnc);
  void record_state_change(TypeHandle type);
  pset<TextureContext *> _current_textures;
  pset<GeomContext *> _current_geoms;
  pset<GeomNodeContext *> _current_geom_nodes;
#else
  INLINE void init_frame_pstats() { }
  INLINE void add_to_texture_record(TextureContext *) { }
  INLINE void add_to_geom_record(GeomContext *) { }
  INLINE void add_to_geom_node_record(GeomNodeContext *) { }
  INLINE void record_state_change(TypeHandle) { }
  INLINE void count_node(Node *) { }
#endif

protected:
  class StateInfo {
  public:
    INLINE StateInfo(TypeHandle type);
    INLINE StateInfo(const NodeTransitions::value_type &value);
    INLINE StateInfo(const NodeTransitionCache::value_type &value);
    INLINE bool operator < (const StateInfo &other) const;

    TypeHandle _type;
    PT(NodeTransition) _trans;
  };
  typedef pvector<StateInfo> State;
  State _state;

  CPT(RenderState) _qpstate;
  CPT(TransformState) _transform;

  int _buffer_mask;
  Colorf _color_clear_value;
  float _depth_clear_value;
  bool _stencil_clear_value;
  Colorf _accum_clear_value;
  int _clear_buffer_type;

  int _display_region_stack_level;
  int _frame_buffer_stack_level;
  int _lens_stack_level;

  GraphicsWindow *_win;
  PT(RenderTraverser) _render_traverser;

  // These must be set by render_scene().
  Node *_current_root_node;
  LensNode *_current_camera;
  CPT(DisplayRegion) _current_display_region;

  CPT(Lens) _current_lens;

  // This is used by wants_normals()
  bool _normals_enabled;

  CoordinateSystem _coordinate_system;

  Colorf _scene_graph_color;
  bool _has_scene_graph_color;
  bool _issued_color_stale;
  bool _vertex_colors_enabled;

  bool _color_transform_enabled;
  bool _alpha_transform_enabled;
  LMatrix4f _current_color_mat;
  float _current_alpha_offset;
  float _current_alpha_scale;

public:
  // Statistics
  static PStatCollector _total_texusage_pcollector;
  static PStatCollector _active_texusage_pcollector;
  static PStatCollector _total_geom_pcollector;
  static PStatCollector _active_geom_pcollector;
  static PStatCollector _total_geom_node_pcollector;
  static PStatCollector _active_geom_node_pcollector;
  static PStatCollector _total_texmem_pcollector;
  static PStatCollector _used_texmem_pcollector;
  static PStatCollector _texmgrmem_total_pcollector;
  static PStatCollector _texmgrmem_resident_pcollector;
  static PStatCollector _vertices_pcollector;
  static PStatCollector _vertices_tristrip_pcollector;
  static PStatCollector _vertices_trifan_pcollector;
  static PStatCollector _vertices_tri_pcollector;
  static PStatCollector _vertices_other_pcollector;
  static PStatCollector _state_changes_pcollector;
  static PStatCollector _transform_state_pcollector;
  static PStatCollector _texture_state_pcollector;
  static PStatCollector _nodes_pcollector;
  static PStatCollector _geom_nodes_pcollector;
  static PStatCollector _frustum_cull_volumes_pcollector;
  static PStatCollector _frustum_cull_transforms_pcollector;
  static PStatCollector _set_state_pcollector;
  static PStatCollector _draw_primitive_pcollector;

private:
  // NOTE: on win32 another DLL (e.g. libpandadx.dll) cannot access
  // these sets directly due to exported template issue
  typedef pset<TextureContext *> Textures;
  Textures _prepared_textures;  
  typedef pset<GeomContext *> Geoms;
  Geoms _prepared_geoms;  
  typedef pset<GeomNodeContext *> GeomNodes;
  GeomNodes _prepared_geom_nodes;  

public:
  void traverse_prepared_textures(bool (*pertex_callbackfn)(TextureContext *,void *),void *callback_arg);

// factory stuff
public:
  typedef Factory<GraphicsStateGuardian> GsgFactory;
  typedef FactoryParam GsgParam;

  // Make a factory parameter type for the window pointer
  class EXPCL_PANDA GsgWindow : public GsgParam {
  public:
    INLINE GsgWindow(GraphicsWindow* w) : GsgParam(), _w(w) {}
    virtual ~GsgWindow(void);
    INLINE GraphicsWindow* get_window(void) { return _w; }
  public:
    static TypeHandle get_class_type(void);
    static void init_type(void);
    virtual TypeHandle get_type(void) const;
    virtual TypeHandle force_init_type(void);
  private:
    GraphicsWindow* _w;
    static TypeHandle _type_handle;

    INLINE GsgWindow(void) : GsgParam() {}
  };

  static GsgFactory &get_factory();

private:
  static void read_priorities(void);

  static GsgFactory *_factory;

public:
  INLINE GraphicsWindow* get_window(void) const { return _win; }

public:
  static TypeHandle get_class_type() {
    return _type_handle;
  }

public:
  static void init_type() {
    GraphicsStateGuardianBase::init_type();
    register_type(_type_handle, "GraphicsStateGuardian",
                  GraphicsStateGuardianBase::get_class_type());
  }
  virtual TypeHandle get_type() const {
    return get_class_type();
  }
  virtual TypeHandle force_init_type() {init_type(); return get_class_type();}

private:
  static TypeHandle _type_handle;

  friend class GraphicsPipe;
  friend class GraphicsWindow;
};

template<class Transition>
INLINE bool
get_attribute_into(Transition *&ptr, const GraphicsStateGuardian *gsg);

#include "graphicsStateGuardian.I"

#endif
