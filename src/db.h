/* ************************************************************************
*   File: db.h                                          Part of Bylins    *
*  Usage: header file for database handling                               *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
*                                                                         *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                       *
************************************************************************ */

#ifndef _DB_H_
#define _DB_H_

#include "obj.hpp"
#include "boot.constants.hpp"
#include "structs.h"
#include "conf.h"	// to get definition of build type: (CIRCLE_AMIGA|CIRCLE_UNIX|CIRCLE_WINDOWS|CIRCLE_ACORN|CIRCLE_VMS)

#include <boost/array.hpp>
#include <map>

struct ROOM_DATA;	// forward declaration to avoid inclusion of room.hpp and any dependencies of that header.
class CHAR_DATA;	// forward declaration to avoid inclusion of char.hpp and any dependencies of that header.

// room manage functions
void room_copy(ROOM_DATA * dst, ROOM_DATA * src);
void room_free(ROOM_DATA * room);

// public procedures in db.cpp
void tag_argument(char *argument, char *tag);
void boot_db(void);
int create_entry(const char *name);
void zone_update(void);
bool can_be_reset(zone_rnum zone);
room_rnum real_room(room_vnum vnum);
long get_id_by_name(char *name);
long get_id_by_uid(long uid);
int get_uid_by_id(int id);
long cmp_ptable_by_name(char *name, int len);
const char *get_name_by_id(long id);
char* get_name_by_unique(int unique);
int get_level_by_unique(long unique);
long get_lastlogon_by_unique(long unique);
long get_ptable_by_unique(long unique);
int get_zone_rooms(int, int *, int *);

int load_char(const char *name, CHAR_DATA * char_element, bool reboot = 0);
void init_char(CHAR_DATA *ch);
CHAR_DATA *read_mobile(mob_vnum nr, int type);
mob_rnum real_mobile(mob_vnum vnum);
int vnum_mobile(char *searchname, CHAR_DATA * ch);
void clear_char_skills(CHAR_DATA * ch);
int correct_unique(int unique);
bool check_unlimited_timer(const CObjectPrototype* obj);

#define REAL          0
#define VIRTUAL       (1 << 0)
#define OBJ_NO_CALC   (1 << 1)

void free_obj(OBJ_DATA * obj);
CObjectPrototype::shared_ptr get_object_prototype(obj_vnum nr, int type = VIRTUAL);

int vnum_object(char *searchname, CHAR_DATA * ch);
int vnum_flag(char *searchname, CHAR_DATA * ch);
int vnum_room(char *searchname, CHAR_DATA * ch);

struct object_criterion
{
	std::map<std::string, float> affects, param, extra;
	float weight, weight_value, miw, timer_value, timer_step;
};


// structure for the reset commands
struct reset_com
{
	char command;		// current command

	int if_flag;		// 4 modes of command execution
	int arg1;		//
	int arg2;		// Arguments to the command
	int arg3;		//
	int arg4;
	int line;		// line number this command appears on
	char *sarg1;		// string argument
	char *sarg2;		// string argument

	/*
	 *  Commands:              *
	 *  'M': Read a mobile     *
	 *  'O': Read an object    *
	 *  'G': Give obj to mob   *
	 *  'P': Put obj in obj    *
	 *  'G': Obj to char       *
	 *  'E': Obj to char equip *
	 *  'D': Set state of door *
	 *  'T': Trigger command   *
	 */
};

struct _case
{
	// ���� �������
	int vnum;
	// ���� ����������
	int chance;
	// ����� ������, ������� �������� �� �����
	std::vector<int> vnum_objs;
};

// ��� ������������� � random_obj
struct ExtraAffects
{
	int number; // ����� �������������
	int min_val; // ����������� ��������
	int max_val; // ������������ ��������
	int chance; // ����������� ����, ��� ������ ����������� ����� �� ������
};



struct City
{
		std::string name; // ��� ������
		std::vector<int> vnums; // ������ ���, ������� ����������� ������
		int rent_vnum; // ���� ����� ������
};


class RandomObj
{
	public:
		// ���� �������
		int vnum;
		// ������, � ������� ������������, ���� ������ ���������� + ����, ��� ��� "�������������" ��� ��������� �������� ����� �� ���
		std::map<std::string, int> not_wear;
		// ����������� � ������������ ���
		int min_weight;
		int max_weight;
		// ����������� � ������������ ���� �� �������
		int min_price;
		int max_price;
		// ���������
		int min_stability;
		int max_stability;
		// value0, value1, value2, value3
		int value0_min, value1_min, value2_min, value3_min;
		int value0_max, value1_max, value2_max, value3_max;
		// ������ �������� � �� ���� ������ �� ������
		std::map<std::string, int> affects;
		// ������ ������������� � �� ���� ������ �� ������
		std::vector<ExtraAffects> extraffect;
};

// zone definition structure. for the 'zone-table'
struct zone_data
{
	char *name;		// name of this zone
	// �����, ����...
	char *comment;
//MZ.load
	int level;	// level of this zone (is used in ingredient loading)
	int type;	// the surface type of this zone (is used in ingredient loading)
//-MZ.load

	int lifespan;		// how long between resets (minutes)
	int age;		// current age of this zone (minutes)
	room_vnum top;		// upper limit for rooms in this zone

	int reset_mode;		// conditions for reset (see below)
	zone_vnum number;	// virtual number of this zone
	// �������������� ����
	char *location;
	// �������� ����
	char *description;
	struct reset_com *cmd;	// command table for reset

	/*
	 * Reset mode:
	 *   0: Don't reset, and don't update age.
	 *   1: Reset if no PC's are located in zone.
	 *   2: Just reset.
	 *   3: Multi reset.
	 */
	int typeA_count;
	int *typeA_list;	// ������ ������� ���, ������� ������������ ������������ � ����
	int typeB_count;
	int *typeB_list;	// ������ ������� ���, ������� ������������ ����������, �� ��� ������ ���� �������� �� ������ ��� ���� �
	bool *typeB_flag;	// �����, ���� �� �������� ���� ���� �
	int under_construction;	// ���� � �������� ������������
	bool locked;
	bool reset_idle;	// ������� �� ����, � ������� ����� �� �����
	bool used;		// ��� �� ���-�� � ���� ����� �������
	unsigned long long activity;	// �������� ���������� ������� � ����
	// <= 1 - ������� ���� (����), >= 2 - ���� ��� ������ �� ���������� ���-�� �������
	int group;
	// ������� ������� ����� � ����
	int mob_level;
	// �������� �� ���� �������
	bool is_town;
	// ���������� ���������� ������� ����, ��� �������, ��� � ���� �����
	int count_reset;
};

extern zone_data *zone_table;

// for queueing zones for update
struct reset_q_element
{
	zone_rnum zone_to_reset;	// ref to zone_data
	struct reset_q_element *next;
};

// structure for the update queue
struct reset_q_type
{
	struct reset_q_element *head;
	struct reset_q_element *tail;
};

#define OBJECT_SAVE_ACTIVITY 300
#define PLAYER_SAVE_ACTIVITY 300
#define MAX_SAVED_ITEMS      1000

struct player_index_element
{
	char *name;
	//added by WorM ������������ ��� ���� � ��������� ����
	char *mail;
	char *last_ip;
	//end by WorM
	int id;
	int unique;
	int level;
	int last_logon;
	int activity;		// When player be saved and checked
	save_info *timer;
};

struct Route
{
	std::string direction;
	int wait;
};

struct SpeedWalk
{
	int default_wait;
	std::vector<int> vnum_mobs;
	std::vector<Route> route;
	int cur_state;
	int wait;
	std::vector<CHAR_DATA *> mobs;
};

#define SEASON_WINTER		0
#define SEASON_SPRING		1
#define SEASON_SUMMER		2
#define SEASON_AUTUMN		3

#define MONTH_JANUARY   	0
#define MONTH_FEBRUARY  	1
#define MONTH_MART			2
#define MONTH_APRIL			3
#define MONTH_MAY			4
#define MONTH_JUNE			5
#define MONTH_JULY			6
#define MONTH_AUGUST		7
#define MONTH_SEPTEMBER		8
#define MONTH_OCTOBER		9
#define MONTH_NOVEMBER		10
#define MONTH_DECEMBER		11
#define DAYS_PER_WEEK		7

struct month_temperature_type
{
	int min;
	int max;
	int med;
};

//Polud �������� ����� ��� �������� ���������� ��������� ��� �����
struct ingredient
{
	int imtype;
	std::string imname;
	boost::array<int, MAX_MOB_LEVEL + 1> prob; // ����������� �������� ��� ������� ������ ����
};

class MobRace{
public:
	MobRace();
	~MobRace();
	std::string race_name;
	std::vector<ingredient> ingrlist;
};

typedef boost::shared_ptr<MobRace> MobRacePtr;
typedef std::map<int, MobRacePtr> MobRaceListType;

//-Polud

extern room_rnum top_of_world;

#ifndef __CONFIG_C__
extern char const *OK;
extern char const *NOPERSON;
extern char const *NOEFFECT;
#endif

// external variables

extern const int sunrise[][2];
extern const int Reverse[];

// external vars
extern CHAR_DATA *combat_list;

#include <vector>
#include <deque>

class CRooms: public std::deque<ROOM_DATA *>
{
public:
	static constexpr int UNDEFINED_ROOM_VNUM = -1;
};

extern CRooms world;
extern CHAR_DATA *character_list;
extern INDEX_DATA *mob_index;
extern mob_rnum top_of_mobt;
extern int top_of_p_table;

inline obj_vnum GET_OBJ_VNUM(const CObjectPrototype* obj) { return obj->get_vnum(); }

extern DESCRIPTOR_DATA *descriptor_list;
extern CHAR_DATA *mob_proto;
extern const char *MENU;

extern struct portals_list_type *portals_list;
extern TIME_INFO_DATA time_info;

extern int convert_drinkcon_skill(CObjectPrototype *obj, bool proto);

int dl_parse(load_list ** dl_list, char *line);
int dl_load_obj(OBJ_DATA * corpse, CHAR_DATA * ch, CHAR_DATA * chr, int DL_LOAD_TYPE);
int trans_obj_name(OBJ_DATA * obj, CHAR_DATA * ch);
void dl_list_copy(load_list * *pdst, load_list * src);
void paste_mobiles();

extern room_rnum r_helled_start_room;
extern room_rnum r_mortal_start_room;
extern room_rnum r_immort_start_room;
extern room_rnum r_named_start_room;
extern room_rnum r_unreg_start_room;

long get_ptable_by_name(const char *name);
void free_alias(struct alias_data *a);
extern player_index_element* player_table;

bool player_exists(const long id);

inline save_info* SAVEINFO(const size_t number)
{
	return player_table[number].timer;
}

inline void clear_saveinfo(const size_t number)
{
	delete player_table[number].timer;
	player_table[number].timer = NULL;
}

void recreate_saveinfo(const size_t number);

void set_god_skills(CHAR_DATA *ch);
void check_room_flags(int rnum);

namespace OfftopSystem
{
	void init();
	void set_flag(CHAR_DATA *ch);
} // namespace OfftopSystem

extern int now_entrycount;

void delete_char(const char *name);

void set_test_data(CHAR_DATA *mob);

extern zone_rnum top_of_zone_table;
void set_zone_mob_level();

bool can_snoop(CHAR_DATA *imm, CHAR_DATA *vict);

extern insert_wanted_gem iwg;

class GameLoader
{
public:
	GameLoader();

	void boot_world();
	void index_boot(const EBootType mode);

private:
	static void prepare_global_structures(const EBootType mode, const int rec_count);
};

extern GameLoader world_loader;

#endif

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
