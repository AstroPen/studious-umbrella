#ifndef _NAR_ENTITY_CPP_
#define _NAR_ENTITY_CPP_


static inline Entity *alloc_entity(GameState *g) {
  if (g->num_entities == g->max_entities) return NULL;
  Entity *result = g->entities + g->num_entities;
  *result = {};
  g->num_entities++;
  return result;
}

// TODO this is really slow, I'd rather use a free list but that would break
// the iterator, so it will have to wait
static inline void free_entity(GameState *g, Entity *e) {
  assert(g->num_entities > 0);
  auto last_entity = g->entities + g->num_entities - 1;
  if (e != last_entity) *e = *last_entity;
  g->num_entities--;
}

inline void make_sprite(Entity *e) {
  e->flags |= ENTITY_SPRITE | ENTITY_TEXTURE;
  if (!e->visual.scale) e->visual.scale = 1;
  if (e->visual.color == vec4(0)) e->visual.color = vec4(1,1,1,1);
}

inline void set_texture(Entity *e, BitmapID texture_id) {
  e->flags |= ENTITY_TEXTURE;
  e->visual.texture_id = texture_id;
}

inline void set_normal_map(Entity *e, BitmapID normal_map_id) {
  e->flags |= ENTITY_NORMAL_MAP;
  e->visual.normal_map_id = normal_map_id;
}

inline void make_moving(Entity *e) {
  e->flags |= ENTITY_MOVING;
  e->friction_multiplier = 1.0f;
}

inline void set_friction_multiplier(Entity *e, float friction) {
  e->flags |= ENTITY_MOVING;
  e->friction_multiplier = friction;
}

inline void set_lifetime(Entity *e, float lifetime) {
  e->flags |= ENTITY_TEMPORARY;
  e->lifetime = lifetime;
}

inline void set_bounce_factor(Entity *e, float bounce_factor) {
  if (!(e->flags & ENTITY_MOVING)) make_moving(e);
  e->flags |= ENTITY_BOUNCING;
  e->bounce_factor = bounce_factor;
}

inline void set_slip_factor(Entity *e, float slip_factor) {
  if (!(e->flags & ENTITY_MOVING)) make_moving(e);
  e->flags |= ENTITY_SLIDING;
  e->slip_factor = slip_factor;
}

// TODO rename these to "projectile" or "bullet" or something
static inline Entity *add_particle(GameState *g, V2 position, V2 velocity, float mass, float radius, float lifetime, V4 color) {
  TIMED_FUNCTION();

  auto e = alloc_entity(g);
  if (!e) return NULL;

  e->flags |= ENTITY_COLLIDES;
  e->collision_box = aligned_box(aligned_rect(position, 2*radius, 2*radius), 0.5 - radius, radius * 2);
  e->mass = mass;
  set_bounce_factor(e, 1);
  e->vel.xy = velocity;
  set_lifetime(e, lifetime);

  auto circle_texture = get_bitmap(&g->assets, BITMAP_CIRCLE);
  float texture_radius = circle_texture->height * METERS_PER_PIXEL / 2;

  make_sprite(e);
  set_texture(e, BITMAP_CIRCLE);
  set_normal_map(e, BITMAP_SPHERE_NORMAL_MAP);
  e->visual.color = color;
  e->visual.scale = radius / texture_radius;

  return e;
}

// TODO change this to something reasonable
#define INFINITE_MASS 1000000.0f;

static Entity *add_wall(GameState *g, AlignedRect rect, V4 color) {
  Entity *wall = alloc_entity(g);
  if (!wall) return NULL;
  wall->flags = ENTITY_COLLIDES | ENTITY_SOLID | ENTITY_CUBOID | ENTITY_TEXTURE | ENTITY_TEXTURE_REPEAT | ENTITY_NORMAL_MAP;
  wall->mass = INFINITE_MASS;
  wall->visual.color = color;
  wall->visual.texture_id = BITMAP_WALL;
  wall->visual.normal_map_id = BITMAP_WALL_NORMAL_MAP;
  wall->visual.scale = 2.0f;
  //wall->collision_box = rect;
  wall->collision_box = aligned_box(rect, 0, 1);
  return wall;
}

static bool add_room(GameState *g, AlignedRect tile_bounds) {
  //auto tile_size = V2{1,1};
  auto wall_color = vec4(0.2, 0.2, 0.2, 1);

  add_wall(g, aligned_rect(min_xy(tile_bounds), 
                           V2{max_x(tile_bounds), min_y(tile_bounds) + 1}), 
                           wall_color);

  add_wall(g, aligned_rect(V2{min_x(tile_bounds), max_y(tile_bounds) - 1}, 
                           max_xy(tile_bounds)), 
                           wall_color);

  add_wall(g, aligned_rect(min_xy(tile_bounds), 
                           V2{min_x(tile_bounds) + 1, max_y(tile_bounds)}), 
                           wall_color);

  add_wall(g, aligned_rect(V2{max_x(tile_bounds) - 1, min_y(tile_bounds)}, 
                           max_xy(tile_bounds)), 
                           wall_color);
  return true;
}

static Entity *add_player(GameState *g, V3 start_p, AnimationType start_animation, Direction facing_direction) {
  float player_size = 80.0f * METERS_PER_PIXEL;

  Entity *player = alloc_entity(g);
  assert(player);
  player->collision_box = aligned_box(aligned_rect(start_p.xy, player_size, player_size / 1.5), 0, 1.3);
  player->flags |= 
    ENTITY_COLLIDES | 
    ENTITY_PLAYER_CONTROLLED | 
    ENTITY_SOLID;

  make_sprite(player);
  set_texture(player, BITMAP_LINK);
  set_normal_map(player, BITMAP_LINK_NORMAL_MAP);
  player->visual.offset = vec3(0, 0.40, 0.06);
  player->visual.scale = 4.0;
  player->visual.sprite_depth = 0.3;

  set_friction_multiplier(player, 0.87);
  set_bounce_factor(player, 0.1);
  set_slip_factor(player, 0.95);

  player->mass = 1.0f;
  player->accel_max = 300.0f;
  player->time_to_max_accel = 0.2f;
  player->time_to_zero_accel = 0.1f;

  player->texture_group_id = TEXTURE_GROUP_LINK;
  player->facing_direction = facing_direction;
  player->animation_dt = 0;
  player->current_animation = start_animation;
  player->animation_duration = get_animation_duration(&g->assets, player);
  return player;
}

#if 0
static inline void push_cuboid_entity(RenderBuffer *render_buffer, Entity *e) {
  auto elem = push_element(render_buffer, RenderElementBox);
  elem->box = e->collision_box;
  elem->color = e->visual.color;
  elem->texture = e->visual.texture;
  /*
  auto box = e->collision_box;
  AlignedRect3 faces[6] = {bottom_face(box), back_face(box), left_face(box), right_face(box), front_face(box), top_face(box)};
  V4 colors[6] = {{0.5,0.5,0.5,1},{0.6,0.6,0.6,1},{0.7,0.7,0.7,1},{0.8,0.l,0.8,1},{0.9,0.9,0.9,1},{1.0,1.0,1.0,1}};

  for (int i = 0; i < 6; i++) {
  //for (int i = 3; i < 4; i++) {
    auto face = faces[i];
    //printf("rect : (%f, %f, %f), (%f, %f, %f)\n", face.center.x, face.center.y, face.center.z, face.offset.x, face.offset.y, face.offset.z);
    if (e->flags & ENTITY_TEXTURE) {
      auto elem = push_element(render_buffer, RenderElementTexture);
      // TODO this will always scale the texture to the face, which we may not want...
      elem->rect = rectangle(face);
      elem->texture = e->visual.texture;
      elem->color = e->visual.color * colors[i];
    } else {
      auto elem = push_element(render_buffer, RenderElementRect);
      elem->rect = rectangle(face);
      elem->color = e->visual.color * colors[i];
    }
  }
  */
}
#endif

// TODO make this less bad please
static inline void push_entity(GameState *g, RenderBuffer *render_buffer, Entity *e) {
  TIMED_FUNCTION();

  if (e->flags & ENTITY_PLAYER_CONTROLLED) {
    render_entity(render_buffer, e); 
    return;
  }

  u32 normal_map_id = 0;
  if (e->flags & ENTITY_NORMAL_MAP) {
    auto normal_map = get_bitmap(&g->assets, e->visual.normal_map_id);
    if (normal_map) normal_map_id = normal_map->texture_id;
  }
  u32 texture_id;
  BitmapID texture_asset_id = BITMAP_WHITE;
  PixelBuffer *texture = NULL;

  if (e->flags & ENTITY_TEXTURE) {
    texture = get_bitmap(&g->assets, e->visual.texture_id);
    if (!texture) return;
    texture_id = texture->texture_id;
    texture_asset_id = e->visual.texture_id;
  } else {
    texture = get_bitmap(&g->assets, BITMAP_WHITE);
    assert(texture);
    texture_id = texture->texture_id;
  }

  if (e->flags & ENTITY_CUBOID) {
    if (e->flags & ENTITY_TEXTURE_REPEAT) {
      push_box(render_buffer, e->collision_box, e->visual.color, texture_asset_id, e->visual.scale, normal_map_id);
      return;
    }

    // TODO TODO FIXME Turn this back on
    //push_box(render_buffer, e->collision_box, e->visual.color, texture->texture_id);
    assert(false);
    return;
  }


  if (e->flags & ENTITY_SPRITE) { 
    // NOTE : push_sprite is depreciated, sprites should be drawn using render_entity
    //push_sprite(render_buffer, e->collision_box, e->visual);

  } else {

    auto r = rectangle(flatten(e->collision_box), e->collision_box.center.z);

    push_rectangle(render_buffer, r, e->visual.color, texture_asset_id, normal_map_id);
  }

}

static void push_entities(RenderBuffer *render_buffer, GameState *g) {
  for (int i = 0; i < g->num_entities; i++) {
    push_entity(g, render_buffer, g->entities + i);
  }
}


#if 0
static void update_position(Entity *e, V2 dp) {
  switch (e->shape_type) {
    case SHAPE_ALIGNED_RECT : {
      auto *rect = &e->collision_box.a_rect;
      translate(rect, dp);
    } break;

    case SHAPE_CIRCLE : {
      auto *circ = &e->collision_box.circ;
      circ->center += dp;
    } break;
  }
}
#endif

// TODO change this so that we sum the shapes first
#if 0
static inline Collision2 find_intersect(Entity *a, Entity *b, float dt) {
  Motion2 move;
  move.a = e->acc;
  move.v = e->vel;
  move.dt = dt;

  switch (e->shape_type) {
    case SHAPE_ALIGNED_RECT : {
      AlignedRect *rect = &e->collision_box.a_rect;
      auto collision = rect_intersect(e->collision_box.a_rect, move, other->collision_box.a_rect, SOLID_INTERIOR);
    } break;

    case SHAPE_CIRCLE : {
      Circle *circ = &e->collision_box.circ;
    } break;
  }
}
#endif

inline Direction to_direction(V2 v) {
  if (!non_zero(v)) return DIRECTION_INVALID;
  if (v.y) {
    if (v.y > 0) return UP;
    else return DOWN;
  } else {
    if (v.x > 0) return RIGHT;
    else return LEFT;
  }
}

// TODO this should also handle translation and other changes
static void update_entity(GameState *g, Entity *e, V3 dp, float dt) {
#define DEBUG_ANIMAIONS 0
  translate(&e->collision_box, dp);

  if (e->flags & ENTITY_PLAYER_CONTROLLED) {
    // TODO This should maybe be handled by a separate animation system.
    float vel_multiplier = length(e->vel.xy) / LINK_RUN_SPEED;
    assert(vel_multiplier >= 0);
    e->animation_dt += dt * vel_multiplier;
    if (e->animation_dt > e->animation_duration) e->animation_dt -= e->animation_duration;

    bool is_moving = vel_multiplier > (0.7 / LINK_RUN_SPEED);
    bool is_pushing = e->pressed_direction != DIRECTION_INVALID;

    if (is_moving && is_pushing) e->current_animation = ANIM_MOVE;
    else if (is_moving) e->current_animation = ANIM_SLIDE;
    else e->current_animation = ANIM_IDLE;

    e->animation_duration = get_animation_duration(&g->assets, e);
#if DEBUG_ANIMAIONS
    switch (e->current_animation) {
      case ANIM_IDLE : e->visual.color  = vec4(1.1, 1.1, 0.5, 1); break;
      case ANIM_MOVE : e->visual.color  = vec4(0.6, 0.6, 1.8, 1); break;
      case ANIM_SLIDE : e->visual.color = vec4(1.8, 0.6, 0.6, 1); break;

      case ANIM_COUNT : e->visual.color   = vec4(2.0, 2.0, 2.0, 1); break;
      case ANIM_INVALID : e->visual.color = vec4(0.1, 0.1, 0.1, 1); break;
    }
#endif  
  }

  if(e->flags & ENTITY_TEMPORARY) {
    e->lifetime -= dt;
  }
#undef DEBUG_ANIMAIONS
}

// TODO consider renaming/inlining this
static void process_entity(GameState *g, Entity *e, Entity *collider_list, int num_colliders, float dt) {
  TIMED_FUNCTION();

  // TODO generalize this to work with circles

  if (e->flags & ENTITY_PLAYER_CONTROLLED) {
    //e->visual.color = g->player_color;

    float acc_scale = length(e->acc);
    float jerk = e->accel_max / e->time_to_max_accel;
    float dejerk = e->accel_max / e->time_to_zero_accel;
    auto dir = to_direction(g->player_direction);
    e->pressed_direction = dir;

    if (dir != DIRECTION_INVALID) {
      e->facing_direction = dir;

      if (length_sq(e->acc) < squared(e->accel_max)) {
        acc_scale += jerk * dt;
      }
    } else if (length_sq(e->acc) > 0.0) {
      acc_scale -= dejerk * dt;
    }

    acc_scale = clamp(acc_scale, 0.0f, e->accel_max);
    e->acc.xy = g->player_direction * acc_scale;

  }


  assert(e->friction_multiplier);
  e->vel *= e->friction_multiplier;

  if (!non_zero(e->vel.x)) e->vel.x = 0;
  if (!non_zero(e->vel.y)) e->vel.y = 0;

  for (int i = 0; i < 4 && dt; i++) {

    Motion2 move;
    move.a = e->acc.xy;
    move.v = e->vel.xy;
    move.dt = dt;

    auto first_collsion = COLLISION_MISS;
    Entity *collided_entity = NULL;

    for (int idx = 0; idx < num_colliders; idx++) {
      Entity *other = collider_list + idx;
      if (other == e) continue;
      if (!(other->flags & ENTITY_COLLIDES) || !(other->flags & ENTITY_SOLID)) continue;

      // TODO replace this call with a generalized find_intersect function
      auto collision = rect_intersect(flatten(e->collision_box), move, flatten(other->collision_box), SOLID_INTERIOR);
      if (is_earlier_collision(collision, first_collsion)) {
        first_collsion = collision;
        collided_entity = other;
      }
    }

    auto p = center(e->collision_box).xy;
    auto p_start = p;

    auto collision = first_collsion;

    // TODO fix the case where you can have greater than g->force_scale force by bouncing diagonally
    if (collision.t >= 0 && collision.t <= dt) {
      assert(collided_entity);
      p = collision.pos;
      e->vel.xy = collision.vel;
      auto acc_scale = length(e->acc);

      // NOTE I used to have a special case for instant collisions, but it was too sensitive and
      // caused bounces to randomly not happen whenever the motion lined up just right.

      if (dot(e->vel.xy, collision.normal) < 0) {
        // Collided with correct side of wall :
        e->vel.xy = reflect(e->vel.xy, collision.normal);
        //printf("b\n");

        // TODO v - ((bd+1)*p) / bd
        // proj_a (x) = dot(x,a) / dot(a,a) * a
        auto projection = dot(e->vel.xy, collision.normal) * collision.normal;
        e->vel.xy -= projection;
        e->vel.xy += projection * e->bounce_factor;
        auto accel_projection = dot(e->acc.xy, collision.normal) * collision.normal;
        e->acc.xy -= accel_projection;
        e->acc = normalize(e->acc) * acc_scale * e->slip_factor;
        // TODO use a second divsior here instead of just 5
        //e->acc += accel_projection / 5;
      }

      auto dp = vec3(p - p_start, 0);
      update_entity(g, e, dp, dt);
        
      dt -= collision.t;
      if (e->flags & ENTITY_PLAYER_CONTROLLED) g->collision_normal = collision.normal;

    } else {
      auto dp = e->vel * dt + 0.5f * e->acc * dt * dt;
      e->vel += e->acc * dt;

      update_entity(g, e, dp, dt);

      dt = 0;
      break;
    }
  }

}

#endif // _NAR_ENTITY_CPP_
