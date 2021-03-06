/*
Minetest
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef LOCALPLAYER_HEADER
#define LOCALPLAYER_HEADER

#include "player.h"
#include "environment.h"
#include "constants.h"
#include <list>

class Client;
class Environment;
class GenericCAO;
class ClientActiveObject;
class ClientEnvironment;
class IGameDef;
struct collisionMoveResult;

enum LocalPlayerAnimations
{
	NO_ANIM,
	WALK_ANIM,
	DIG_ANIM,
	WD_ANIM
}; // no local animation, walking, digging, both

class LocalPlayer : public Player
{
public:
	LocalPlayer(Client *client, const char *name);
	virtual ~LocalPlayer();

	ClientActiveObject *parent;

	// Initialize hp to 0, so that no hearts will be shown if server
	// doesn't support health points
	u16 hp;
	bool isAttached;
	bool touching_ground;
	// This oscillates so that the player jumps a bit above the surface
	bool in_liquid;
	// This is more stable and defines the maximum speed of the player
	bool in_liquid_stable;
	// Gets the viscosity of water to calculate friction
	u8 liquid_viscosity = 0;
	bool is_climbing = false;
	bool swimming_vertical = false;
	bool swimming_pitch = false;

	float physics_override_speed = 1.0f;
	float physics_override_jump = 1.0f;
	float physics_override_gravity = 1.0f;
	bool physics_override_sneak = true;
	bool physics_override_sneak_glitch = false;
	// Temporary option for old move code
	bool physics_override_new_move;

	v3f overridePosition;

	void move(f32 dtime, Environment *env, f32 pos_max_d);
	void move(f32 dtime, Environment *env, f32 pos_max_d,
			std::vector<CollisionInfo> *collision_info);
	// Temporary option for old move code
	void old_move(f32 dtime, Environment *env, f32 pos_max_d,
			std::vector<CollisionInfo> *collision_info);

	void applyControl(float dtime, ClientEnvironment *env);

	v3s16 getStandingNodePos();
	v3s16 getFootstepNodePos();

	// Used to check if anything changed and prevent sending packets if not
	v3f last_position;
	v3f last_speed;
	float last_pitch;
	float last_yaw;
	unsigned int last_keyPressed;
	u8 last_camera_fov;
	u8 last_wanted_range;

	float camera_impact;

	bool makes_footstep_sound;

	int last_animation;
	float last_animation_speed;

	std::string hotbar_image;
	std::string hotbar_selected_image;

	video::SColor light_color;

	float hurt_tilt_timer;
	float hurt_tilt_strength;

	GenericCAO *getCAO() const { return m_cao; }

	void setCAO(GenericCAO *toset)
	{
		assert(!m_cao); // Pre-condition
		m_cao = toset;
	}

	u32 maxHudId() const { return hud.size(); }

	u16 getBreath() const { return m_breath; }
	void setBreath(u16 breath) { m_breath = breath; }

	v3s16 getLightPosition() const;

	void setYaw(f32 yaw) { m_yaw = yaw; }

	f32 getYaw() const { return m_yaw; }

	void setPitch(f32 pitch) { m_pitch = pitch; }

	f32 getPitch() const { return m_pitch; }

	inline void setPosition(const v3f &position)
	{
		m_position = position;
		m_sneak_node_exists = false;
	}

	v3f getPosition() const { return m_position; }
	v3f getEyePosition() const { return m_position + getEyeOffset(); }
	v3f getEyeOffset() const;

	bool getAutojump() const { return m_autojump; }

private:
	void accelerate(const v3f &target_speed, const f32 max_increase_H,
			const f32 max_increase_V, const bool use_pitch);
	bool updateSneakNode(Map *map, const v3f &position, const v3f &sneak_max);
	float getSlipFactor(Environment *env, const v3f &speedH);
	void handleAutojump(f32 dtime, Environment *env,
			const collisionMoveResult &result,
			const v3f &position_before_move, const v3f &speed_before_move,
			f32 pos_max_d);

	v3f m_position;
	v3s16 m_standing_node;

	v3s16 m_sneak_node;
	// Stores the top bounding box of m_sneak_node
	aabb3f m_sneak_node_bb_top;
	// Whether the player is allowed to sneak
	bool m_sneak_node_exists;
	// Whether a "sneak ladder" structure is detected at the players pos
	// see detectSneakLadder() in the .cpp for more info (always false if disabled)
	bool m_sneak_ladder_detected;

	// ***** Variables for temporary option of the old move code *****
	// Stores the max player uplift by m_sneak_node
	f32 m_sneak_node_bb_ymax;
	// Whether recalculation of m_sneak_node and its top bbox is needed
	bool m_need_to_get_new_sneak_node;
	// Node below player, used to determine whether it has been removed,
	// and its old type
	v3s16 m_old_node_below;
	std::string m_old_node_below_type;
	// ***** End of variables for temporary option *****

	bool m_can_jump;
	u16 m_breath;
	f32 m_yaw;
	f32 m_pitch;
	bool camera_barely_in_ceiling;
	aabb3f m_collisionbox;

	bool m_autojump = false;
	float m_autojump_time = 0.0f;

	GenericCAO *m_cao;
	Client *m_client;
};

#endif
