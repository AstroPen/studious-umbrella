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

// TODO rename these to "projectile" or "bullet" or something
static inline Entity *add_particle(GameState *g, V2 position, V2 velocity, float mass, float radius, float lifetime, V4 color) {
  TIMED_FUNCTION();

  auto e = alloc_entity(g);
  if (!e) return NULL;
  e->flags = ENTITY_COLLIDES | ENTITY_BOUNCING | ENTITY_MOVING | ENTITY_TEXTURE | ENTITY_TEMPORARY | ENTITY_NORMAL_MAP;
  //e->collision_box = aligned_rect(position, 2*radius, 2*radius);
  e->collision_box = aligned_box(aligned_rect(position, 2*radius, 2*radius), 0.5 - radius, radius * 2);
  e->vel.xy = velocity;
  // e->acc = 0;
  e->mass = mass;
  e->lifetime = lifetime;
  e->visual.color = color;
  e->visual.texture_id = BITMAP_CIRCLE;
  e->visual.normal_map_id = BITMAP_SPHERE_NORMAL_MAP;
  e->friction_multiplier = 1.0;
  e->bounce_factor = 1.0;
  //e->slip_factor = 1.0;
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
  auto wall_color = V4{0.2, 0.2, 0.2, 1};

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

  uint32_t normal_map_id = 0;
  if (e->flags & ENTITY_NORMAL_MAP) normal_map_id = get_bitmap(&g->assets, e->visual.normal_map_id)->texture_id;

  if (e->flags & ENTITY_CUBOID) {
    auto texture = get_bitmap(&g->assets, e->visual.texture_id);
    if (e->flags & ENTITY_TEXTURE_REPEAT) {
      float tex_width = texture->width * METERS_PER_PIXEL * e->visual.scale;
      float tex_height = texture->height * METERS_PER_PIXEL * e->visual.scale;
      push_box(render_buffer, e->collision_box, e->visual.color, texture->texture_id, tex_width, tex_height, normal_map_id);
      return;
    }

    // TODO TODO FIXME Turn this back on
    //push_box(render_buffer, e->collision_box, e->visual.color, texture->texture_id);
    assert(false);
    return;
  }

  if (e->flags & ENTITY_TEXTURE) {

    if (e->flags & ENTITY_SPRITE) {
      auto texture = get_bitmap(&g->assets, e->visual.texture_id);
      if (!texture) return;

      float width  = texture->width  * METERS_PER_PIXEL * e->visual.scale;
      float height = texture->height * METERS_PER_PIXEL * e->visual.scale;

      Rectangle r;
      auto offset = e->visual.offset;
      offset.z = 0;
      r.center = center(e->collision_box) + e->visual.offset;
      r.center.z -= e->collision_box.offset.z;
      r.offsets[1].z = r.offsets[0].z = 0; //e->collision_box.offset.z; //e->visual.sprite_height;
      r.offsets[0].x = width / 2;
      r.offsets[1].x = -width / 2;
      r.offsets[0].y = height / 2;
      r.offsets[1].y = height / 2;
      
      push_sprite(render_buffer, r, e->visual.offset.z, e->visual.sprite_height, e->visual.color, texture->texture_id, normal_map_id);

    } else {

      auto texture = get_bitmap(&g->assets, e->visual.texture_id);
      auto r = rectangle(flatten(e->collision_box), e->collision_box.center.z);

      push_rectangle(render_buffer, r, e->visual.color, texture->texture_id, normal_map_id);
    }

  } else {
    auto r = rectangle(flatten(e->collision_box), e->collision_box.center.z);
    push_rectangle(render_buffer, r, e->visual.color, get_bitmap(&g->assets, BITMAP_WHITE)->texture_id, normal_map_id);
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

// TODO consider renaming/inlining this
static void process_entity(GameState *g, Entity *e, Entity *collider_list, int num_colliders, float dt) {
  TIMED_FUNCTION();

  // TODO generalize this to work with circles

  if (e->flags & ENTITY_PLAYER_CONTROLLED) {
    //e->visual.color = g->player_color;

    float acc_scale = length(e->acc);
    float jerk = e->accel_max / e->time_to_max_accel;
    float dejerk = e->accel_max / e->time_to_zero_accel;
    if (g->player_direction != V2{0,0}) {
      if (length_sq(e->acc) < squared(e->accel_max)) {
        acc_scale += jerk * dt;
      }
    } else if (length_sq(e->acc) > 0.0) {
      acc_scale -= dejerk * dt;
    }

    acc_scale = clamp(acc_scale, 0.0f, e->accel_max);
    e->acc.xy = g->player_direction * acc_scale;
  }

  if(e->flags & ENTITY_TEMPORARY) {
    e->lifetime -= dt;
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
        
      dt -= collision.t;
      if (e->flags & ENTITY_PLAYER_CONTROLLED) g->collision_normal = collision.normal;
      auto dp = p - p_start;
      translate(&e->collision_box, v3(dp,0));

    } else {
      auto dp = e->vel * dt + 0.5f * e->acc * dt * dt;
      e->vel += e->acc * dt;
      translate(&e->collision_box, dp);
      dt = 0;
      break;
    }
  }

  /*
  if(e->flags & ENTITY_TEMPORARY && e->lifetime <= 0) {
    e->flags = 0;
    e->collision_box = {};
  }
  */

  //assert(dt < PHYSICS_FRAME_TIME);

  //auto dp = e->vel * dt + 0.5f * e->acc * dt * dt;
  //e->vel += e->acc * dt;

  //e->collision_box.min_pos += dp;
  //e->collision_box.max_pos += dp;
}

#endif // _NAR_ENTITY_CPP_
