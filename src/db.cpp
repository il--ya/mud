/************************************************************************
*   File: db.cpp                                        Part of Bylins    *
*  Usage: Loading/saving chars, booting/resetting world, internal funcs   *
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

#define __DB_C__

#include "db.h"

#include "object.prototypes.hpp"
#include "world.objects.hpp"
#include "logger.hpp"
#include "utils.h"
#include "shutdown.parameters.hpp"
#include "boards.h"
#include "ban.hpp"
#include "birth_places.hpp"
#include "bonus.h"
#include "boot.data.files.hpp"
#include "boot.index.hpp"
#include "celebrates.hpp"
#include "char.hpp"
#include "corpse.hpp"
#include "deathtrap.hpp"
#include "depot.hpp"
#include "dg_scripts.h"
#include "ext_money.hpp"
#include "fight.h"
#include "file_crc.hpp"
#include "glory.hpp"
#include "glory_const.hpp"
#include "glory_misc.hpp"
#include "handler.h"
#include "help.hpp"
#include "house.h"
#include "item.creation.hpp"
#include "liquid.hpp"
#include "mail.h"
#include "mob_stat.hpp"
#include "modify.h"
#include "named_stuff.hpp"
#include "noob.hpp"
#include "olc.h"
#include "parcel.hpp"
#include "parse.hpp"
#include "player_races.hpp"
#include "privilege.hpp"
#include "sets_drop.hpp"
#include "shop_ext.hpp"
#include "stuff.hpp"
#include "time_utils.hpp"
#include "title.hpp"
#include "top.h"
#include "class.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

#include <sys/stat.h>

#include <sstream>
#include <string>
#include <cmath>

#define CRITERION_FILE "criterion.xml"
#define CRITERIONS_FILE "criterions.xml"
#define CASES_FILE "cases.xml"
#define RANDOMOBJ_FILE "randomobj.xml"
#define SPEEDWALKS_FILE "speedwalks.xml"
#define CLASS_LIMIT_FILE "class_limit.xml"
#define DAILY_FILE "daily.xml"
#define CITIES_FILE "cities.xml"
/**************************************************************************
*  declarations of global containers and objects                          *
**************************************************************************/
BanList *ban = 0;		// contains list of banned ip's and proxies + interface

/**************************************************************************
*  declarations of most of the 'global' variables                         *
**************************************************************************/
#if defined(CIRCLE_MACINTOSH)
long beginning_of_time = -1561789232;
#else
long beginning_of_time = 650336715;
#endif

CRooms world;

room_rnum top_of_world = 0;	// ref to top element of world

CHAR_DATA *character_list = NULL;	// global linked list of chars

INDEX_DATA **trig_index;	// index table for triggers
int top_of_trigt = 0;		// top of trigger index table

INDEX_DATA *mob_index;		// index table for mobile file
mob_rnum top_of_mobt = 0;	// top of mobile index table
void Load_Criterion(pugi::xml_node XMLCriterion, int type);
void load_speedwalk();
void load_class_limit();
int global_uid = 0;

struct zone_data *zone_table;	// zone table
zone_rnum top_of_zone_table = 0;	// top element of zone tab
struct message_list fight_messages[MAX_MESSAGES];	// fighting messages
extern int slot_for_char(CHAR_DATA * ch, int slot_num);
struct player_index_element *player_table = NULL;	// index to plr file

bool player_exists(const long id)
{
	if (id == -1)
	{
		return true;
	}

	if (0 == top_of_p_table)
	{
		return false;
	}

	for (auto i = 0; i <= top_of_p_table; i++)
	{
		if (id == player_table[i].id)
		{
			return true;
		}
	}

	return false;
}

FILE *player_fl = NULL;		// file desc of player file
int top_of_p_table = 0;		// ref to top of table
int top_of_p_file = 0;		// ref of size of p file
long top_idnum = 0;		// highest idnum in use

int circle_restrict = 0;	// level of game restriction
room_rnum r_mortal_start_room;	// rnum of mortal start room
room_rnum r_immort_start_room;	// rnum of immort start room
room_rnum r_frozen_start_room;	// rnum of frozen start room
room_rnum r_helled_start_room;
room_rnum r_named_start_room;
room_rnum r_unreg_start_room;

char *credits = NULL;		// game credits
char *motd = NULL;		// message of the day - mortals
char *rules = NULL;		// rules for immorts
char *GREETINGS = NULL;		// opening credits screen
char *help = NULL;		// help screen
char *info = NULL;		// info page
char *immlist = NULL;		// list of peon gods
char *background = NULL;	// background story
char *handbook = NULL;		// handbook for new immortals
char *policies = NULL;		// policies page
char *name_rules = NULL;		// rules of character's names

TIME_INFO_DATA time_info;	// the infomation about the time
struct weather_data weather_info;	// the infomation about the weather
struct reset_q_type reset_q;	// queue of zones to be reset

const FLAG_DATA clear_flags;

struct portals_list_type *portals_list;	// ������ �������� ��� townportal
int now_entrycount = FALSE;

extern int number_of_social_messages;
extern int number_of_social_commands;

guardian_type guardian_list;

insert_wanted_gem iwg;

GameLoader::GameLoader()
{
}

GameLoader world_loader;

// local functions
void SaveGlobalUID(void);
void LoadGlobalUID(void);
void parse_room(FILE * fl, int virtual_nr, int virt);
void parse_object(FILE * obj_f, const int nr);
void load_help(FILE * fl);
void assign_mobiles(void);
void assign_objects(void);
void assign_rooms(void);
void init_spec_procs(void);
void build_player_index(void);
int is_empty(zone_rnum zone_nr);
void reset_zone(zone_rnum zone);
int file_to_string(const char *name, char *buf);
int file_to_string_alloc(const char *name, char **buf);
void do_reboot(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void check_start_rooms(void);
void renum_world(void);
void renum_zone_table(void);
void log_zone_error(zone_rnum zone, int cmd_no, const char *message);
void reset_time(void);
int mobs_in_room(int m_num, int r_num);
void new_build_player_index(void);
void renum_obj_zone(void);
void renum_mob_zone(void);
int get_zone_rooms(int, int *, int *);
void init_guilds(void);
void init_basic_values(void);
void init_portals(void);
void init_im(void);
void init_zone_types();
void load_guardians();

// external functions
TIME_INFO_DATA *mud_time_passed(time_t t2, time_t t1);
void free_alias(struct alias_data *a);
void load_messages(void);
void weather_and_time(int mode);
void mag_assign_spells(void);
void sort_commands(void);
void Read_Invalid_List(void);
int find_name(const char *name);
int csort(const void *a, const void *b);
void prune_crlf(char *txt);
int Crash_read_timer(int index, int temp);
void Crash_clear_objects(int index);
void extract_mob(CHAR_DATA * ch);
//F@N|
int exchange_database_load(void);

void create_rainsnow(int *wtype, int startvalue, int chance1, int chance2, int chance3);
void calc_easter(void);
void do_start(CHAR_DATA * ch, int newbie);
int calc_loadroom(CHAR_DATA * ch, int bplace_mode = BIRTH_PLACE_UNDEFINED);
extern void repop_decay(zone_rnum zone);	// ���������� �������� ITEM_REPOP_DECAY
int real_zone(int number);
int level_exp(CHAR_DATA * ch, int level);
extern void NewNameRemove(CHAR_DATA * ch);
extern void NewNameLoad();
extern char *fread_action(FILE * fl, int nr);

//polud
void load_mobraces();
//-polud

// external vars
extern int no_specials;
extern room_vnum mortal_start_room;
extern room_vnum immort_start_room;
extern room_vnum frozen_start_room;
extern room_vnum helled_start_room;
extern room_vnum named_start_room;
extern room_vnum unreg_start_room;
extern DESCRIPTOR_DATA *descriptor_list;
extern struct month_temperature_type year_temp[];
extern const char *pc_class_types[];
extern char *house_rank[];
extern struct pclean_criteria_data pclean_criteria[];
extern int class_stats_limit[NUM_PLAYER_CLASSES][6];
extern void LoadProxyList();
extern void add_karma(CHAR_DATA * ch, const char * punish , const char * reason);

int strchrn (const char *s, int c) {
	
 int n=-1;
 while (*s) {
  n++;
  if (*s==c) return n;
  s++;
 }
 return -1;
}


// ����� ���������
int strchrs(const char *s, const char *t) { 
 while (*t) {
  int r=strchrn(s,*t);
  if (r>-1) return r;
  t++;
 }
 return -1;
}


struct Item_struct
{
	// ��� ����������
	std::map<std::string, double> params;
	// ��� ��������
	std::map<std::string, double> affects;
};

// ������ ��� ��������, ������ ������� ������� ��� ��������� ����
Item_struct items_struct[17]; // = new Item_struct[16];

// ����������� ������� ������
int exp_two_implementation(int number)
{
	int count = 0;
	while (true)
	{
		const int tmp = 1 << count;
		if (number < tmp)
		{
			return -1;
		}
		if (number == tmp)
		{
			return count;
		}
		count++;
	}
}

template <typename EnumType>
inline int exp_two(EnumType number)
{
	return exp_two_implementation(to_underlying(number));
}

template <> inline int exp_two(int number) { return exp_two_implementation(number); }

bool check_obj_in_system_zone(int vnum)
{
    // �����
    if ((vnum < 400) && (vnum > 299))
	return true;
    // ���-����
    if ((vnum >= 1200) && (vnum <= 1299))
	return true; 
    if ((vnum >= 2300) && (vnum <= 2399))
	return true;
    // ����
    if ((vnum >= 1500) && (vnum <= 1599))
	return true;
    return false;
}

bool check_unlimited_timer(const CObjectPrototype* obj)
{
	char buf_temp[MAX_STRING_LENGTH];
	char buf_temp1[MAX_STRING_LENGTH];
	//sleep(15);
	// ���� ��������� ��� �������
	int item_wear = -1;
	bool type_item = false;
	if (GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_ARMOR
		|| GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_STAFF
		|| GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_WORN
		|| GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_WEAPON)
	{
		type_item = true;
	}
	// ����� ��� ������
	double sum = 0;
	// ����� ��� ��������
	double sum_aff = 0;
	// �� ������� ��� �� ����������
	if (obj->has_wear_flag(EWearFlag::ITEM_WEAR_FINGER))
	{
		item_wear = exp_two(EWearFlag::ITEM_WEAR_FINGER);
	}

	if (obj->has_wear_flag(EWearFlag::ITEM_WEAR_NECK))
	{
		item_wear = exp_two(EWearFlag::ITEM_WEAR_NECK);
	}

	if (obj->has_wear_flag(EWearFlag::ITEM_WEAR_BODY))
	{
		item_wear = exp_two(EWearFlag::ITEM_WEAR_BODY);
	}

	if (obj->has_wear_flag(EWearFlag::ITEM_WEAR_HEAD))
	{
		item_wear = exp_two(EWearFlag::ITEM_WEAR_HEAD);
	}

	if (obj->has_wear_flag(EWearFlag::ITEM_WEAR_LEGS))
	{
		item_wear = exp_two(EWearFlag::ITEM_WEAR_LEGS);
	}

	if (obj->has_wear_flag(EWearFlag::ITEM_WEAR_FEET))
	{
		item_wear = exp_two(EWearFlag::ITEM_WEAR_FEET);
	}

	if (obj->has_wear_flag(EWearFlag::ITEM_WEAR_HANDS))
	{
		item_wear = exp_two(EWearFlag::ITEM_WEAR_HANDS);
	}

	if (obj->has_wear_flag(EWearFlag::ITEM_WEAR_ARMS))
	{
		item_wear = exp_two(EWearFlag::ITEM_WEAR_ARMS);
	}

	if (obj->has_wear_flag(EWearFlag::ITEM_WEAR_SHIELD))
	{
		item_wear = exp_two(EWearFlag::ITEM_WEAR_SHIELD);
	}

	if (obj->has_wear_flag(EWearFlag::ITEM_WEAR_ABOUT))
	{
		item_wear = exp_two(EWearFlag::ITEM_WEAR_ABOUT);
	}

	if (obj->has_wear_flag(EWearFlag::ITEM_WEAR_WAIST))
	{
		item_wear = exp_two(EWearFlag::ITEM_WEAR_WAIST);
	}

	if (obj->has_wear_flag(EWearFlag::ITEM_WEAR_WRIST))
	{
		item_wear = exp_two(EWearFlag::ITEM_WEAR_WRIST);
	}

	if (obj->has_wear_flag(EWearFlag::ITEM_WEAR_WIELD))
	{
		item_wear = exp_two(EWearFlag::ITEM_WEAR_WIELD);
	}

	if (obj->has_wear_flag(EWearFlag::ITEM_WEAR_HOLD))
	{
		item_wear = exp_two(EWearFlag::ITEM_WEAR_HOLD);
	}

	if (obj->has_wear_flag(EWearFlag::ITEM_WEAR_BOTHS))
	{
		item_wear = exp_two(EWearFlag::ITEM_WEAR_BOTHS);
	}

	if (!type_item)
	{
		return false;
	}
	// ��������� �� ������ � ��������� ����
	if (check_obj_in_system_zone(obj->get_vnum()))
	{
		return false;
	}
	// ���� ������ ������ �� ����������, �� ���, �����
	if (item_wear == -1)
	{
		return false;
	}
	// ���� ������ ���������� ��� ���������� ������ �������
	if (obj->get_extra_flag(EExtraFlag::ITEM_MAGIC))
	{
		return false;
	}
	// ���� ��� ������� �������
	if (obj->get_extra_flag(EExtraFlag::ITEM_SETSTUFF))
	{
		return false;
	}
	// ���������� ��� ����
	if (obj->get_extra_flag(EExtraFlag::ITEM_ZONEDECAY))
	{
		return false;
	}
	// ���������� �� ����� ����
	if (obj->get_extra_flag(EExtraFlag::ITEM_REPOP_DECAY))
	{
		return false;
	}

	// ���� ������� ������� �������, �� �� ���� ����
	if (obj->get_auto_mort_req() > 0)
		return false;
	if (obj->get_minimum_remorts() > 0)
		return false;
	// ��������� ����� � ��������
	 if (obj->get_extra_flag(EExtraFlag::ITEM_WITH1SLOT)
		 || obj->get_extra_flag(EExtraFlag::ITEM_WITH2SLOTS)
		 || obj->get_extra_flag(EExtraFlag::ITEM_WITH3SLOTS))
	 {
		 return false;
	 }
	// ���� � ������� ������ ����, �� �����. 
	if (obj->get_timer() == 0)
	{
		return false;
	}
	// �������� �� ���� ��������������� ��������
	for (int i = 0; i < MAX_OBJ_AFFECT; i++)
	{
		if (obj->get_affected(i).modifier)
		{
			sprinttype(obj->get_affected(i).location, apply_types, buf_temp);
			// �������� �� ����� ������� � ����������
			for (std::map<std::string, double>::iterator it = items_struct[item_wear].params.begin(); it != items_struct[item_wear].params.end(); it++)
			{
				if (strcmp(it->first.c_str(), buf_temp) == 0)
				{
					if (obj->get_affected(i).modifier > 0)
					{
						sum += it->second * obj->get_affected(i).modifier;
					}
				}
			}
		}
	}
	obj->get_affect_flags().sprintbits(weapon_affects, buf_temp1, ",");
	
	// �������� �� ���� �������� � ����� �������
	for(std::map<std::string, double>::iterator it = items_struct[item_wear].affects.begin(); it != items_struct[item_wear].affects.end(); it++) 
	{
		// ���������, ���� �� ��� ������ �� ��������
		if (strstr(buf_temp1, it->first.c_str()) != NULL)
		{
			sum_aff += it->second;
		}
		//std::cout << it->first << " " << it->second << std::endl;
	}

	// ���� ����� ������ ��� ����� �������
	if (sum >= 1)
	{
		return false;
	}

	// ���� ����� ��� �������� �� �������
	if (sum_aff >= 1)
	{
		return false;
	}

	// ����� ��� ����
	return true;
}

float count_koef_obj(const CObjectPrototype *obj,int item_wear)
{
	double sum = 0.0;
	double sum_aff = 0.0;
	char buf_temp[MAX_STRING_LENGTH];
	char buf_temp1[MAX_STRING_LENGTH];

	// �������� �� ���� ��������������� ��������
	for (int i = 0; i < MAX_OBJ_AFFECT; i++)
	{
		if (obj->get_affected(i).modifier)
		{
			sprinttype(obj->get_affected(i).location, apply_types, buf_temp);
			// �������� �� ����� ������� � ����������
			for (std::map<std::string, double>::iterator it = items_struct[item_wear].params.begin(); it != items_struct[item_wear].params.end(); it++)
			{

				if (strcmp(it->first.c_str(), buf_temp) == 0)
				{
					if (obj->get_affected(i).modifier > 0)
					{
						sum += it->second * obj->get_affected(i).modifier;
					}
				}

				//std::cout << it->first << " " << it->second << std::endl;
			}
		}
	}
	obj->get_affect_flags().sprintbits(weapon_affects, buf_temp1, ",");
	
	// �������� �� ���� �������� � ����� �������
	for(std::map<std::string, double>::iterator it = items_struct[item_wear].affects.begin(); it != items_struct[item_wear].affects.end(); it++) 
	{
		// ���������, ���� �� ��� ������ �� ��������
		if (strstr(buf_temp1, it->first.c_str()) != NULL)
		{
			sum_aff += it->second;
		}
		//std::cout << it->first << " " << it->second << std::endl;
	}
	sum += sum_aff;
	return sum;
}

float count_unlimited_timer(const CObjectPrototype *obj)
{
	float result = 0.0;
	bool type_item = false;
	if (GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_ARMOR
		|| GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_STAFF
		|| GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_WORN
		|| GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_WEAPON)
	{
		type_item = true;
	}
	// ����� ��� ������
	
	result = 0.0;
	
	if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_FINGER))
	{
		result += count_koef_obj(obj, exp_two(EWearFlag::ITEM_WEAR_FINGER));
	}

	if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_NECK))
	{
		result += count_koef_obj(obj, exp_two(EWearFlag::ITEM_WEAR_NECK));
	}

	if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_BODY))
	{
		result += count_koef_obj(obj, exp_two(EWearFlag::ITEM_WEAR_BODY));
	}

	if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_HEAD))
	{
		result += count_koef_obj(obj, exp_two(EWearFlag::ITEM_WEAR_HEAD));
	}

	if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_LEGS))
	{
		result += count_koef_obj(obj, exp_two(EWearFlag::ITEM_WEAR_LEGS));
	}

	if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_FEET))
	{
		result += count_koef_obj(obj, exp_two(EWearFlag::ITEM_WEAR_FEET));
	}

	if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_HANDS))
	{
		result += count_koef_obj(obj, exp_two(EWearFlag::ITEM_WEAR_HANDS));
	}

	if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_ARMS))
	{
		result += count_koef_obj(obj, exp_two(EWearFlag::ITEM_WEAR_ARMS));
	}

	if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_SHIELD))
	{
		result += count_koef_obj(obj, exp_two(EWearFlag::ITEM_WEAR_SHIELD));
	}

	if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_ABOUT))
	{
		result += count_koef_obj(obj, exp_two(EWearFlag::ITEM_WEAR_ABOUT));
	}

	if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_WAIST))
	{
		result += count_koef_obj(obj, exp_two(EWearFlag::ITEM_WEAR_WAIST));
	}

	if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_WRIST))
	{
		result += count_koef_obj(obj, exp_two(EWearFlag::ITEM_WEAR_WRIST));
	}

	if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_WIELD))
	{
		result += count_koef_obj(obj, exp_two(EWearFlag::ITEM_WEAR_WIELD));
	}

	if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_HOLD))
	{
		result += count_koef_obj(obj, exp_two(EWearFlag::ITEM_WEAR_HOLD));
	}

	if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_BOTHS))
	{
		result += count_koef_obj(obj, exp_two(EWearFlag::ITEM_WEAR_BOTHS));
	}

	if (!type_item)
	{
		return 0.0;
	}

	return result;
}

float count_mort_requred(const CObjectPrototype *obj)
{
    
	float result = 0.0;
	const float SQRT_MOD = 1.7095f;
	const int AFF_SHIELD_MOD = 30;
	const int AFF_MAGICGLASS_MOD = 10;
	const int AFF_BLINK_MOD = 10;

	result = 0.0;
	
	if (ObjSystem::is_mob_item(obj)
		|| OBJ_FLAGGED(obj, EExtraFlag::ITEM_SETSTUFF))
	{
		return result;
	}

	float total_weight = 0.0;

	// ������� APPLY_x
	for (int k = 0; k < MAX_OBJ_AFFECT; k++)
	{
		if (obj->get_affected(k).location == 0) continue;

		// ������, ���� ���� ������ �������� � ���������� �����
		for (int kk = 0; kk < MAX_OBJ_AFFECT; kk++)
		{
			if (obj->get_affected(k).location == obj->get_affected(kk).location
				&& k != kk)
			{
				log("SYSERROR: double affect=%d, obj_vnum=%d",
					obj->get_affected(k).location, GET_OBJ_VNUM(obj));
				return 1000000;
			}
		}
		if ((obj->get_affected(k).modifier > 0)&&((obj->get_affected(k).location != APPLY_AC)&&
                        (obj->get_affected(k).location != APPLY_SAVING_WILL)&&
                        (obj->get_affected(k).location != APPLY_SAVING_CRITICAL)&&
                        (obj->get_affected(k).location != APPLY_SAVING_STABILITY)&&
                        (obj->get_affected(k).location != APPLY_SAVING_REFLEX)))
		{
                    float weight = ObjSystem::count_affect_weight(obj, obj->get_affected(k).location, obj->get_affected(k).modifier);
        	    log("SYSERROR: negative weight=%f, obj_vnum=%d",
					weight, GET_OBJ_VNUM(obj));
                    total_weight += pow(weight, SQRT_MOD);
		}
                // ������ ������� � ������� ������ ����� �������� ��� ���� � +
 		else if ((obj->get_affected(k).modifier > 0)&&((obj->get_affected(k).location == APPLY_AC)||
                        (obj->get_affected(k).location == APPLY_SAVING_WILL)||
                        (obj->get_affected(k).location == APPLY_SAVING_CRITICAL)||
                        (obj->get_affected(k).location == APPLY_SAVING_STABILITY)||
                        (obj->get_affected(k).location == APPLY_SAVING_REFLEX)))
		{
                    float weight = ObjSystem::count_affect_weight(obj, obj->get_affected(k).location, 0-obj->get_affected(k).modifier);
                    total_weight -= pow(weight, -SQRT_MOD);
		}
               //���������� ����� ���� ������� � - ����������
                else if ((obj->get_affected(k).modifier < 0)
                        &&((obj->get_affected(k).location == APPLY_AC)||
                        (obj->get_affected(k).location == APPLY_SAVING_WILL)||
                        (obj->get_affected(k).location == APPLY_SAVING_CRITICAL)||
                        (obj->get_affected(k).location == APPLY_SAVING_STABILITY)||
                        (obj->get_affected(k).location == APPLY_SAVING_REFLEX)))
                {
                    float weight = ObjSystem::count_affect_weight(obj, obj->get_affected(k).location, obj->get_affected(k).modifier);
                    total_weight += pow(weight, SQRT_MOD);
                }
               //���������� ����� ���� �������������� �������� �� �� �������
                else if ((obj->get_affected(k).modifier < 0)
                        &&((obj->get_affected(k).location != APPLY_AC)&&
                        (obj->get_affected(k).location != APPLY_SAVING_WILL)&&
                        (obj->get_affected(k).location != APPLY_SAVING_CRITICAL)&&
                        (obj->get_affected(k).location != APPLY_SAVING_STABILITY)&&
                        (obj->get_affected(k).location != APPLY_SAVING_REFLEX)))
                {
                    float weight = ObjSystem::count_affect_weight(obj, obj->get_affected(k).location, 0-obj->get_affected(k).modifier);
                    total_weight -= pow(weight, -SQRT_MOD);
                }
	}
	// ������� AFF_x ����� weapon_affect
	for (const auto& m : weapon_affect)
	{
		if (IS_OBJ_AFF(obj, m.aff_pos))
		{
			if (static_cast<EAffectFlag>(m.aff_bitvector) == EAffectFlag::AFF_AIRSHIELD)
			{
				total_weight += pow(AFF_SHIELD_MOD, SQRT_MOD);
			}
			else if (static_cast<EAffectFlag>(m.aff_bitvector) == EAffectFlag::AFF_FIRESHIELD)
			{
				total_weight += pow(AFF_SHIELD_MOD, SQRT_MOD);
			}
			else if (static_cast<EAffectFlag>(m.aff_bitvector) == EAffectFlag::AFF_ICESHIELD)
			{
				total_weight += pow(AFF_SHIELD_MOD, SQRT_MOD);
			}
			else if (static_cast<EAffectFlag>(m.aff_bitvector) == EAffectFlag::AFF_MAGICGLASS)
			{
				total_weight += pow(AFF_MAGICGLASS_MOD, SQRT_MOD);
			}
			else if (static_cast<EAffectFlag>(m.aff_bitvector) == EAffectFlag::AFF_BLINK)
			{
				total_weight += pow(AFF_BLINK_MOD, SQRT_MOD);
			}
		}
	}
        if (total_weight < 1) return result;
	
        result = ceil(pow(total_weight, 1/SQRT_MOD));

	return result;
    
}

void Load_Criterion(pugi::xml_node XMLCriterion, const EWearFlag type)
{
	int index = exp_two(type);
	pugi::xml_node params, CurNode, affects;
	params = XMLCriterion.child("params");
	affects = XMLCriterion.child("affects");
	
	// ��������� � ������ ��� ���������, ���� ����, ��������, ���� � ��
	for (CurNode = params.child("param"); CurNode; CurNode = CurNode.next_sibling("param"))
	{
		items_struct[index].params.insert(std::make_pair(CurNode.attribute("name").value(), CurNode.attribute("value").as_double()));
		//log("str:%s, double:%f", CurNode.attribute("name").value(),  CurNode.attribute("value").as_double());
	}
	
	// ��������� � ������ ��� �������
	for (CurNode = affects.child("affect"); CurNode; CurNode = CurNode.next_sibling("affect"))
	{
		items_struct[index].affects.insert(std::make_pair(CurNode.attribute("name").value(), CurNode.attribute("value").as_double()));
		//log("Affects:str:%s, double:%f", CurNode.attribute("name").value(),  CurNode.attribute("value").as_double());
	}	
}

std::map<int, object_criterion> object_criterions;
std::vector<SpeedWalk>  speedwalks;

void load_criterions()
{
	pugi::xml_document document;
	pugi::xml_node object_, file_, child_;
	file_ = XMLLoad(LIB_MISC CRITERIONS_FILE, "criterions_objects", "Error loading cases file: criterions.xml", document);
	for (child_ = file_.child("object");  child_;  child_ = child_.next_sibling("object"))
	{
		object_criterion criterion_tmp;
		const int ind_wear = child_.attribute("index_wear").as_int();
		criterion_tmp.weight = child_.attribute("weight_value").as_float();
		criterion_tmp.timer_step = child_.attribute("timer_step").as_float();
		criterion_tmp.timer_value = child_.attribute("timer_vslue").as_float();
		criterion_tmp.miw = child_.attribute("miw_value").as_float();
		for (object_ = child_.child("param"); object_; object_ = object_.next_sibling("param"))
		{
			criterion_tmp.param[object_.attribute("name").as_string()] = object_.attribute("value").as_float();
		}
		for (object_ = child_.child("extra"); object_; object_ = object_.next_sibling("extra"))
		{
			criterion_tmp.affects[object_.attribute("name").as_string()] = object_.attribute("value").as_float();
		}
		for (object_ = child_.child("affect"); object_; object_ = object_.next_sibling("affect"))
		{
			criterion_tmp.affects[object_.attribute("name").as_string()] = object_.attribute("value").as_float();
		}
		object_criterions[ind_wear] = criterion_tmp;
	}
}


float get_criterion_object(OBJ_DATA *obj)
{
	// ���� ������ ��� ������
	if (GET_OBJ_TYPE(obj) != OBJ_DATA::ITEM_ARMOR)
		return -1000;
	float sum = 0.0;
	float sum_temp = 0.0;
	char buf_temp[MAX_STRING_LENGTH], buf_temp1[MAX_STRING_LENGTH], buf_temp3[MAX_STRING_LENGTH];
	std::vector<int> can_wear;
	
	// �� ������� ����� :(
	if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_FINGER))
	{
		can_wear.push_back(to_underlying(EWearFlag::ITEM_WEAR_FINGER));
	}

	if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_NECK))
	{
		can_wear.push_back(to_underlying(EWearFlag::ITEM_WEAR_NECK));
	}

	if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_BODY))
	{
		can_wear.push_back(to_underlying(EWearFlag::ITEM_WEAR_BODY));
	}

	if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_HEAD))
	{
		can_wear.push_back(to_underlying(EWearFlag::ITEM_WEAR_HEAD));
	}

	if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_LEGS))
	{
		can_wear.push_back(to_underlying(EWearFlag::ITEM_WEAR_LEGS));
	}

	if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_FEET))
	{
		can_wear.push_back(to_underlying(EWearFlag::ITEM_WEAR_FEET));
	}

	if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_HANDS))
	{
		can_wear.push_back(to_underlying(EWearFlag::ITEM_WEAR_HANDS));
	}

	if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_ARMS))
	{
		can_wear.push_back(to_underlying(EWearFlag::ITEM_WEAR_ARMS));
	}

	if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_SHIELD))
	{
		can_wear.push_back(to_underlying(EWearFlag::ITEM_WEAR_SHIELD));
	}

	if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_ABOUT))
	{
		can_wear.push_back(to_underlying(EWearFlag::ITEM_WEAR_ABOUT));
	}

	if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_WAIST))
	{
		can_wear.push_back(to_underlying(EWearFlag::ITEM_WEAR_WAIST));
	}

	if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_WRIST))
	{
		can_wear.push_back(to_underlying(EWearFlag::ITEM_WEAR_WRIST));
	}

	if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_WIELD))
	{
		can_wear.push_back(to_underlying(EWearFlag::ITEM_WEAR_WIELD));

	if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_HOLD))
	{
		can_wear.push_back(to_underlying(EWearFlag::ITEM_WEAR_HOLD));
	}

	if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_BOTHS))
	{
		can_wear.push_back(to_underlying(EWearFlag::ITEM_WEAR_BOTHS));
	}
	
	// ��� ��������� ��������, � �� �������! ������ ���������.
	for (auto i : can_wear)
	{
		if (object_criterions.count(i) > 0)
		{
			sum_temp = 0.0;
			for (int i = 0; i < MAX_OBJ_AFFECT; i++)
			{
				if (obj->get_affected(i).modifier)
				{
					sprinttype(obj->get_affected(i).location, apply_types, buf_temp);
					sum_temp += object_criterions[i].param[buf_temp] * obj->get_affected(i).modifier;
				}
			}
			obj->get_affect_flags().sprintbits(weapon_affects, buf_temp1, ",");
			// � ��� ��� ���� �������
			for (auto it : object_criterions[i].affects)
			{
				// ���������, ���� �� ��� ������ �� ��������
				if (strstr(buf_temp1, it.first.c_str()) != NULL)
				{
					sum_temp += it.second;
				}
			}
			GET_OBJ_EXTRA(obj).sprintbits(extra_bits, buf_temp3, ",");
			for (auto it : object_criterions[i].extra)
			{
				// ���������, ���� �� ��� ������ �� ��������
				if (strstr(buf_temp3, it.first.c_str()) != NULL)
				{
					sum_temp += it.second;
				}
			}
			// ���� ������ ������� ������, ��� timer_value, �� ���������� � ����� ������� ����� �������� � value ���������� 
			// ����.
			if (obj_proto[GET_OBJ_RNUM(obj)]->get_timer() > object_criterions[i].timer_value)
				sum_temp += object_criterions[i].timer_step * GET_OBJ_RNUM(obj)]->get_timer() - object_criterions[i].timer_value;
			if (object_criterions[i].weight > GET_OBJ_WEIGHT(obj))
				sum_temp += object_criterions[i].weight_value;
			if (sum_temp > sum)
				sum = sum_temp;
		}

	}	
	return sum;
}

// Separate a 4-character id tag from the data it precedes
void tag_argument(char *argument, char *tag)
{
	char *tmp = argument, *ttag = tag, *wrt = argument;
	int i;

	for (i = 0; i < 4; i++)
		*(ttag++) = *(tmp++);
	*ttag = '\0';

	while (*tmp == ':' || *tmp == ' ')
		tmp++;

	while (*tmp)
		*(wrt++) = *(tmp++);
	*wrt = '\0';
}

/*************************************************************************
*  routines for booting the system                                       *
*************************************************************************/

void go_boot_socials(void)
{
	int i;

	if (soc_mess_list)
	{
		for (i = 0; i < number_of_social_messages; i++)
		{
			if (soc_mess_list[i].char_no_arg)
				free(soc_mess_list[i].char_no_arg);
			if (soc_mess_list[i].others_no_arg)
				free(soc_mess_list[i].others_no_arg);
			if (soc_mess_list[i].char_found)
				free(soc_mess_list[i].char_found);
			if (soc_mess_list[i].others_found)
				free(soc_mess_list[i].others_found);
			if (soc_mess_list[i].vict_found)
				free(soc_mess_list[i].vict_found);
			if (soc_mess_list[i].not_found)
				free(soc_mess_list[i].not_found);
		}
		free(soc_mess_list);
	}
	if (soc_keys_list)
	{
		for (i = 0; i < number_of_social_commands; i++)
			if (soc_keys_list[i].keyword)
				free(soc_keys_list[i].keyword);
		free(soc_keys_list);
	}
	number_of_social_messages = -1;
	number_of_social_commands = -1;
	world_loader.index_boot(DB_BOOT_SOCIAL);
}

void load_sheduled_reboot()
{
	FILE *sch;
	int day = 0, hour = 0, minutes = 0, numofreaded = 0, timeOffset = 0;
	char str[10];
	int offsets[7];

	time_t currTime;

	sch = fopen(LIB_MISC "schedule", "r");
	if (!sch)
	{
		log("Error opening schedule");
		return;
	}
	time(&currTime);
	for (int i = 0; i < 7; i++)
	{
		if (fgets(str, 10, sch) == NULL)
		{
			break;
		}
		numofreaded = sscanf(str, "%i %i:%i", &day, &hour, &minutes);
		if (numofreaded < 1)
		{
			continue;
		}
		if (day != i)
		{
			break;
		}
		if (numofreaded == 1)
		{
			offsets[i] = 24 * 60;
			continue;
		}
		offsets[i] = hour * 60 + minutes;
	}
	fclose(sch);

	timeOffset = (int) difftime(currTime, shutdown_parameters.get_boot_time());

	//set reboot_uptime equal to current uptime and align to the start of the day
	shutdown_parameters.set_reboot_uptime(timeOffset / 60 - localtime(&currTime)->tm_min - 60 * (localtime(&currTime)->tm_hour));
	const auto boot_time = shutdown_parameters.get_boot_time();
	const auto local_boot_time = localtime(&boot_time);
	for (int i = local_boot_time->tm_wday; i < 7; i++)
	{
		//7 empty days was cycled - break with default uptime
		if (shutdown_parameters.get_reboot_uptime() - 1440 * 7 >= 0)
		{
			shutdown_parameters.set_reboot_uptime(DEFAULT_REBOOT_UPTIME);	//2 days reboot bu default if no schedule
			break;
		}

		//if we get non-1-full-day offset, but server will reboot to early (36 hour is minimum)
		//we are going to find another scheduled day
		if (offsets[i] < 24 * 60 && (shutdown_parameters.get_reboot_uptime() + offsets[i]) < UPTIME_THRESHOLD * 60)
		{
			shutdown_parameters.set_reboot_uptime(shutdown_parameters.get_reboot_uptime() + 24*60);
		}
		//we've found next point of reboot! :) break cycle
		if (offsets[i] < 24*60
			&& (shutdown_parameters.get_reboot_uptime() + offsets[i]) > UPTIME_THRESHOLD * 60)
		{
			shutdown_parameters.set_reboot_uptime(shutdown_parameters.get_reboot_uptime() + offsets[i]);
			break;
		}
		//empty day - add 24 hour and go next
		if (offsets[i] == 24 * 60)
		{
			shutdown_parameters.set_reboot_uptime(shutdown_parameters.get_reboot_uptime() + offsets[i]);
		}
		// jump to 1st day of the week
		if (i == 6)
		{
			i = -1;
		}
	}
	log("Setting up reboot_uptime: %i", shutdown_parameters.get_reboot_uptime());
}

// ������� ������� �������� XML ��������
pugi::xml_node XMLLoad(const char *PathToFile, const char *MainTag, const char *ErrorStr, pugi::xml_document& Doc)
{
	std::ostringstream buffer;
	pugi::xml_parse_result Result;
	pugi::xml_node NodeList;

    // ������� ��������� ����
	Result = Doc.load_file(PathToFile);
	// Oops, ����� ���
	if (!Result)
	{
		buffer << "..." << Result.description();
		mudlog(std::string(buffer.str()).c_str(), CMP, LVL_IMMORT, SYSLOG, TRUE);
		return NodeList;
	}

    // ���� � ����� ��������� ���
	NodeList = Doc.child(MainTag);
	// ���� ��� - ����������� � �������
	if (!NodeList)
	{
		mudlog(ErrorStr, CMP, LVL_IMMORT, SYSLOG, TRUE);
	}

	return NodeList;
};

std::vector<_case> cases;
// ��������� �������� � ����
void load_cases()
{
	pugi::xml_document doc_cases;	
	pugi::xml_node case_, object_, file_case;
	file_case = XMLLoad(LIB_MISC CASES_FILE, "cases", "Error loading cases file: cases.xml", doc_cases);
	for (case_ = file_case.child("casef"); case_; case_ = case_.next_sibling("casef"))
	{
		_case __case;
		__case.vnum = case_.attribute("vnum").as_int();
		__case.chance = case_.attribute("chance").as_int();
		for (object_ = case_.child("object"); object_;  object_ = object_.next_sibling("object"))
		{
			__case.vnum_objs.push_back(object_.attribute("vnum").as_int());
		}
		cases.push_back(__case);
	}
}

std::vector<City> cities;
std::string default_str_cities;
/* �������� ������� �� xml ������ */
void load_cities()
{
	default_str_cities = "";
	pugi::xml_document doc_cities;
	pugi::xml_node child_, object_, file_;
	file_ = XMLLoad(LIB_MISC CITIES_FILE, "cities", "Error loading cases file: cities.xml", doc_cities);
	for (child_ = file_.child("city"); child_; child_ = child_.next_sibling("city"))
	{
		City city;
		city.name = child_.child("name").attribute("value").as_string();
		city.rent_vnum = child_.child("rent_vnum").attribute("value").as_int();
		for (object_ = child_.child("zone_vnum"); object_; object_ = object_.next_sibling("zone_vnum"))
		{
			city.vnums.push_back(object_.attribute("value").as_int());
		}
		cities.push_back(city);
		default_str_cities += "0";
	}
}



std::vector<RandomObj> random_objs;

// �������� ���������� ��� ��������� ������
void load_random_obj()
{
	pugi::xml_document doc_robj;
	pugi::xml_node robj, object, file_robj;
	int tmp_buf2;
	std::string tmp_str;
	file_robj = XMLLoad(LIB_MISC RANDOMOBJ_FILE, "object", "Error loading cases file: random_obj.xml", doc_robj);
	for (robj = file_robj.child("obj"); robj; robj = robj.next_sibling("obj"))
	{
		RandomObj tmp_robj;
		tmp_robj.vnum = robj.attribute("vnum").as_int();
		for (object = robj.child("not_to_wear");object;object = object.next_sibling("not_to_wear"))
		{
			tmp_str = object.attribute("value").as_string();
			tmp_buf2 = object.attribute("chance").as_int();
			tmp_robj.not_wear.insert(std::pair<std::string, int>(tmp_str, tmp_buf2));
		}
		tmp_robj.min_weight = robj.child("weight").attribute("value_min").as_int();
		tmp_robj.max_weight = robj.child("weight").attribute("value_max").as_int();
		tmp_robj.min_price = robj.child("price").attribute("value_min").as_int();
		tmp_robj.max_price = robj.child("price").attribute("value_max").as_int();
		tmp_robj.min_stability = robj.child("stability").attribute("value_min").as_int();
		tmp_robj.max_stability = robj.child("stability").attribute("value_max").as_int();
		tmp_robj.value0_min = robj.child("value_zero").attribute("value_min").as_int();
		tmp_robj.value0_max = robj.child("value_zero").attribute("value_max").as_int();
		tmp_robj.value1_min = robj.child("value_one").attribute("value_min").as_int();
		tmp_robj.value1_max = robj.child("value_one").attribute("value_max").as_int();
		tmp_robj.value2_min = robj.child("value_two").attribute("value_min").as_int();
		tmp_robj.value2_max = robj.child("value_two").attribute("value_max").as_int();
		tmp_robj.value3_min = robj.child("value_three").attribute("value_min").as_int();
		tmp_robj.value3_max = robj.child("value_three").attribute("value_max").as_int();
		for (object = robj.child("affect");object;object = object.next_sibling("affect"))
		{
			tmp_str = object.attribute("value").as_string();
			tmp_buf2 = object.attribute("chance").as_int();
			tmp_robj.affects.insert(std::pair<std::string, int>(tmp_str, tmp_buf2));
		}
		for (object = robj.child("extraaffect");object;object = object.next_sibling("extraaffect"))
		{
			ExtraAffects extraAffect;
			extraAffect.number = object.attribute("value").as_int();
			extraAffect.chance = object.attribute("chance").as_int();
			extraAffect.min_val = object.attribute("min_val").as_int();
			extraAffect.max_val = object.attribute("max_val").as_int();
			tmp_robj.extraffect.push_back(extraAffect);
		}
		random_objs.push_back(tmp_robj);
	}
}

/*
 * Too bad it doesn't check the return values to let the user
 * know about -1 values.  This will result in an 'Okay.' to a
 * 'reload' command even when the string was not replaced.
 * To fix later, if desired. -gg 6/24/99
 */
void do_reboot(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	argument = one_argument(argument, arg);

	if (!str_cmp(arg, "all") || *arg == '*')
	{
		if (file_to_string_alloc(GREETINGS_FILE, &GREETINGS) == 0)
		{
			prune_crlf(GREETINGS);
		}
		file_to_string_alloc(IMMLIST_FILE, &immlist);
		file_to_string_alloc(CREDITS_FILE, &credits);
		file_to_string_alloc(MOTD_FILE, &motd);
		file_to_string_alloc(RULES_FILE, &rules);
		file_to_string_alloc(HELP_PAGE_FILE, &help);
		file_to_string_alloc(INFO_FILE, &info);
		file_to_string_alloc(POLICIES_FILE, &policies);
		file_to_string_alloc(HANDBOOK_FILE, &handbook);
		file_to_string_alloc(BACKGROUND_FILE, &background);
		file_to_string_alloc(NAME_RULES_FILE, &name_rules);
		go_boot_socials();
		init_im();
		init_zone_types();
		init_portals();
		load_sheduled_reboot();
		oload_table.init();
		OBJ_DATA::init_set_table();
		load_mobraces();
		load_morphs();
		GlobalDrop::init();
		OfftopSystem::init();
		Celebrates::load();
		HelpSystem::reload_all();
		Remort::init();
		Noob::init();
		ResetStats::init();
		Bonus::bonus_log_load();
	}
	else if (!str_cmp(arg, "portals"))
		init_portals();
	else if (!str_cmp(arg, "imagic"))
		init_im();
	else if (!str_cmp(arg, "ztypes"))
		init_zone_types();
	else if (!str_cmp(arg, "oloadtable"))
		oload_table.init();
	else if (!str_cmp(arg, "setstuff"))
	{
		OBJ_DATA::init_set_table();
		HelpSystem::reload(HelpSystem::STATIC);
	}
	else if (!str_cmp(arg, "immlist"))
		file_to_string_alloc(IMMLIST_FILE, &immlist);
	else if (!str_cmp(arg, "credits"))
		file_to_string_alloc(CREDITS_FILE, &credits);
	else if (!str_cmp(arg, "motd"))
		file_to_string_alloc(MOTD_FILE, &motd);
	else if (!str_cmp(arg, "rules"))
		file_to_string_alloc(RULES_FILE, &rules);
	else if (!str_cmp(arg, "help"))
		file_to_string_alloc(HELP_PAGE_FILE, &help);
	else if (!str_cmp(arg, "info"))
		file_to_string_alloc(INFO_FILE, &info);
	else if (!str_cmp(arg, "policy"))
		file_to_string_alloc(POLICIES_FILE, &policies);
	else if (!str_cmp(arg, "handbook"))
		file_to_string_alloc(HANDBOOK_FILE, &handbook);
	else if (!str_cmp(arg, "background"))
		file_to_string_alloc(BACKGROUND_FILE, &background);
	else if (!str_cmp(arg, "namerules"))
		file_to_string_alloc(NAME_RULES_FILE, &name_rules);
	else if (!str_cmp(arg, "greetings"))
	{
		if (file_to_string_alloc(GREETINGS_FILE, &GREETINGS) == 0)
			prune_crlf(GREETINGS);
	}
	else if (!str_cmp(arg, "xhelp"))
	{
		HelpSystem::reload_all();
	}
	else if (!str_cmp(arg, "socials"))
		go_boot_socials();
	else if (!str_cmp(arg, "schedule"))
		load_sheduled_reboot();
	else if (!str_cmp(arg, "clan"))
	{
		skip_spaces(&argument);
		if (!*argument)
		{
			Clan::ClanLoad();
			return;
		}
		Clan::ClanListType::iterator clan;
		std::string buffer(argument);
		for (clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan)
		{
			if (CompareParam(buffer, (*clan)->get_abbrev()))
			{
				CreateFileName(buffer);
				Clan::ClanReload(buffer);
				send_to_char("������������ �����.\r\n", ch);
				break;
			}
		}
	}
	else if (!str_cmp(arg, "proxy"))
		LoadProxyList();
	else if (!str_cmp(arg, "boards"))
		Boards::Static::reload_all();
	else if (!str_cmp(arg, "titles"))
		TitleSystem::load_title_list();
	else if (!str_cmp(arg, "emails"))
		RegisterSystem::load();
	else if (!str_cmp(arg, "privilege"))
		Privilege::load();
	else if (!str_cmp(arg, "mobraces"))
		load_mobraces();
	else if (!str_cmp(arg, "morphs"))
		load_morphs();
	else if (!str_cmp(arg, "depot") && PRF_FLAGGED(ch, PRF_CODERINFO))
	{
		skip_spaces(&argument);
		if (*argument)
		{
			long uid = GetUniqueByName(std::string(argument));
			if (uid > 0)
			{
				Depot::reload_char(uid, ch);
			}
			else
			{
				send_to_char("��������� ��� �� ������\r\n"
					"������ �������: reload depot <��� ����>.\r\n", ch);
			}
		}
		else
		{
			send_to_char("������ �������: reload depot <��� ����>.\r\n", ch);
		}
	}
	else if (!str_cmp(arg, "globaldrop"))
	{
		GlobalDrop::init();
	}
	else if (!str_cmp(arg, "offtop"))
	{
		OfftopSystem::init();
	}
	else if (!str_cmp(arg, "shop"))
	{
		ShopExt::load(true);
	}
	else if (!str_cmp(arg, "named"))
	{
		NamedStuff::load();
	}
	else if (!str_cmp(arg, "celebrates"))
	{
		//Celebrates::load(XMLLoad(LIB_MISC CELEBRATES_FILE, CELEBRATES_MAIN_TAG, CELEBRATES_ERROR_STR));
		Celebrates::load();
	}
	else if (!str_cmp(arg, "setsdrop") && PRF_FLAGGED(ch, PRF_CODERINFO))
	{
		skip_spaces(&argument);
		if (*argument && is_number(argument))
		{
			SetsDrop::reload(atoi(argument));
		}
		else
		{
			SetsDrop::reload();
		}
	}
	else if (!str_cmp(arg, "remort.xml"))
	{
		Remort::init();
	}
	else if (!str_cmp(arg, "noob_help.xml"))
	{
		Noob::init();
	}
	else if (!str_cmp(arg, "reset_stats.xml"))
	{
		ResetStats::init();
	}
	else if (!str_cmp(arg, "obj_sets.xml"))
	{
		obj_sets::load();
	}
	else
	{
		send_to_char("�������� �������� ��� ������������ ������.\r\n", ch);
		return;
	}

	std::string str = boost::str(boost::format("%s reload %s.")
		% ch->get_name() % arg);
	mudlog(str.c_str(), NRM, LVL_IMMORT, SYSLOG, TRUE);

	send_to_char(OK, ch);
}

void init_portals(void)
{
	FILE *portal_f;
	char nm[300], nm2[300], *wrd;
	int rnm = 0, i, level = 0;
	struct portals_list_type *curr, *prev;

	// ������� ����������� ��� �������
	for (curr = portals_list; curr; curr = prev)
	{
		prev = curr->next_portal;
		free(curr->wrd);
		free(curr);
	}

	portals_list = NULL;
	prev = NULL;

	// ������ ����
	if (!(portal_f = fopen(LIB_MISC "portals.lst", "r")))
	{
		log("Can not open portals.lst");
		return;
	}

	while (get_line(portal_f, nm))
	{
		if (!nm[0] || nm[0] == ';')
			continue;
		sscanf(nm, "%d %d %s", &rnm, &level, nm2);
		if (real_room(rnm) == NOWHERE || nm2[0] == '\0')
		{
			log("Invalid portal entry detected");
			continue;
		}
		wrd = nm2;
		for (i = 0; !(i == 10 || wrd[i] == ' ' || wrd[i] == '\0'); i++);
		wrd[i] = '\0';
		// ��������� ������ � ������ - rnm - �������, wrd - �����
		CREATE(curr, 1);
		CREATE(curr->wrd, strlen(wrd) + 1);
		curr->vnum = rnm;
		curr->level = level;
		for (i = 0, curr->wrd[i] = '\0'; wrd[i]; i++)
			curr->wrd[i] = LOWER(wrd[i]);
		curr->wrd[i] = '\0';
		curr->next_portal = NULL;
		if (!portals_list)
			portals_list = curr;
		else
			prev->next_portal = curr;

		prev = curr;

	}
	fclose(portal_f);
}

/// ������� ���� GET_OBJ_SKILL � �������� TODO: 12.2013
int convert_drinkcon_skill(CObjectPrototype *obj, bool proto)
{
	if (GET_OBJ_SKILL(obj) > 0
		&& (GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_DRINKCON
			|| GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_FOUNTAIN))
	{
		log("obj_skill: %d - %s (%d)", GET_OBJ_SKILL(obj),
			GET_OBJ_PNAME(obj, 0).c_str(), GET_OBJ_VNUM(obj));
		// ���� �������� ��� ��������� �����-�� �����, �� �����
		// �� ���-����� �� �� �����������, � ������ ���������
		if (obj->get_value(ObjVal::EValueKey::POTION_PROTO_VNUM) < 0)
		{
			const auto potion = world_objects.create_from_prototype_by_vnum(GET_OBJ_SKILL(obj));
			if (potion
				&& GET_OBJ_TYPE(potion) == OBJ_DATA::ITEM_POTION)
			{
				drinkcon::copy_potion_values(potion.get(), obj);
				if (proto)
				{
					// copy_potion_values ����� �� ���� � ���� �� ������,
					// ������� ������� �����, ��� ����� �� ��������
					// ������� �� read_one_object_new ���� ��� ���������
					obj->set_value(ObjVal::EValueKey::POTION_PROTO_VNUM, 0);
				}
			}
		}
		obj->set_skill(SKILL_INVALID);

		return 1;
	}
	return 0;
}

/// ������� ���������� ���������� ����� ����� ���� ������ � �����������
void convert_obj_values()
{
	int save = 0;
	for (const auto i : obj_proto)
	{
		save = std::max(save, convert_drinkcon_skill(i.get(), true));
		if (i->get_extra_flag(EExtraFlag::ITEM_1INLAID))
		{
			i->unset_extraflag(EExtraFlag::ITEM_1INLAID);
			save = 1;
		}
		if (i->get_extra_flag(EExtraFlag::ITEM_2INLAID))
		{
			i->unset_extraflag(EExtraFlag::ITEM_2INLAID);
			save = 1;
		}
		if (i->get_extra_flag(EExtraFlag::ITEM_3INLAID))
		{
			i->unset_extraflag(EExtraFlag::ITEM_3INLAID);
			save = 1;
		}
		// ...
		if (save)
		{
			olc_add_to_save_list(i->get_vnum()/100, OLC_SAVE_OBJ);
			save = 0;
		}
	}
}

void GameLoader::boot_world()
{
	utils::CSteppedProfiler boot_profiler("World booting");

	boot_profiler.next_step("Loading zone table");
	log("Loading zone table.");
	world_loader.index_boot(DB_BOOT_ZON);

	boot_profiler.next_step("Loading triggers");
	log("Loading triggers and generating index.");
	world_loader.index_boot(DB_BOOT_TRG);

	boot_profiler.next_step("Loading rooms");
	log("Loading rooms.");
	world_loader.index_boot(DB_BOOT_WLD);

	boot_profiler.next_step("Renumbering rooms");
	log("Renumbering rooms.");
	renum_world();

	boot_profiler.next_step("Checking start rooms");
	log("Checking start rooms.");
	check_start_rooms();

	boot_profiler.next_step("Loading mobs and regerating index");
	log("Loading mobs and generating index.");
	world_loader.index_boot(DB_BOOT_MOB);

	boot_profiler.next_step("Counting mob's levels");
	log("Count mob quantity by level");
	MobMax::init();

	boot_profiler.next_step("Loading objects");
	log("Loading objs and generating index.");
	world_loader.index_boot(DB_BOOT_OBJ);

	boot_profiler.next_step("Converting deprecated obj values");
	log("Converting deprecated obj values.");
	convert_obj_values();

	boot_profiler.next_step("enumbering zone table");
	log("Renumbering zone table.");
	renum_zone_table();

	boot_profiler.next_step("Renumbering Obj_zone");
	log("Renumbering Obj_zone.");
	renum_obj_zone();

	boot_profiler.next_step("Renumbering Mob_zone");
	log("Renumbering Mob_zone.");
	renum_mob_zone();

	boot_profiler.next_step("Initialization of object rnums");
	log("Init system_obj rnums.");
	system_obj::init();
	log("Init global_drop_obj.");
}

//MZ.load
void init_zone_types()
{
	FILE *zt_file;
	char tmp[1024], dummy[128], name[128], itype_num[128];
	int names = 0;
	int i, j, k, n;

	if (zone_types != NULL)
	{
		for (i = 0; *zone_types[i].name != '\n'; i++)
		{
			if (zone_types[i].ingr_qty > 0)
				free(zone_types[i].ingr_types);

			free(zone_types[i].name);
		}
		free(zone_types[i].name);
		free(zone_types);
		zone_types = NULL;
	}

	zt_file = fopen(LIB_MISC "ztypes.lst", "r");
	if (!zt_file)
	{
		log("Can not open ztypes.lst");
		return;
	}

	while (get_line(zt_file, tmp))
	{
		if (!strn_cmp(tmp, "���", 3))
		{
			if (sscanf(tmp, "%s %s", dummy, name) != 2)
			{
				log("Corrupted file : ztypes.lst");
				return;
			}
			if (!get_line(zt_file, tmp))
			{
				log("Corrupted file : ztypes.lst");
				return;
			}
			if (!strn_cmp(tmp, "����", 4))
			{
				if (tmp[4] != ' ' && tmp[4] != '\0')
				{
					log("Corrupted file : ztypes.lst");
					return;
				}
				for (i = 4; tmp[i] != '\0'; i++)
				{
					if (!a_isdigit(tmp[i]) && !a_isspace(tmp[i]))
					{
						log("Corrupted file : ztypes.lst");
						return;
					}
				}
			}
			else
			{
				log("Corrupted file : ztypes.lst");
				return;
			}
			names++;
		}
		else
		{
			log("Corrupted file : ztypes.lst");
			return;
		}
	}
	names++;

	CREATE(zone_types, names);
	for (i = 0; i < names; i++)
	{
		zone_types[i].name = NULL;
		zone_types[i].ingr_qty = 0;
		zone_types[i].ingr_types = NULL;
	}

	rewind(zt_file);
	i = 0;
	while (get_line(zt_file, tmp))
	{
		sscanf(tmp, "%s %s", dummy, name);
		for (j = 0; name[j] != '\0'; j++)
			if (name[j] == '_')
				name[j] = ' ';
		zone_types[i].name = str_dup(name);
//		if (get_line(zt_file, tmp)); �� ������ ��� ��� ����� ����� -- Krodo
		get_line(zt_file, tmp);
		for (j = 4; tmp[j] != '\0'; j++)
		{
			if (a_isspace(tmp[j]))
				continue;
			zone_types[i].ingr_qty++;
			for (; tmp[j] != '\0' && a_isdigit(tmp[j]); j++);
			j--;
		}
		i++;
	}
	zone_types[i].name = str_dup("\n");

	for (i = 0; *zone_types[i].name != '\n'; i++)
	{
		if (zone_types[i].ingr_qty > 0)
		{
			CREATE(zone_types[i].ingr_types, zone_types[i].ingr_qty);
		}
	}

	rewind(zt_file);
	i = 0;
	while (get_line(zt_file, tmp))
	{
//		if (get_line(zt_file, tmp)); ���������� ����, ��� ����
		get_line(zt_file, tmp);
		for (j = 4, n = 0; tmp[j] != '\0'; j++)
		{
			if (a_isspace(tmp[j]))
				continue;
			for (k = 0; tmp[j] != '\0' && a_isdigit(tmp[j]); j++)
				itype_num[k++] = tmp[j];
			itype_num[k] = '\0';
			zone_types[i].ingr_types[n] = atoi(itype_num);
			n++;
			j--;
		}
		i++;
	}

	fclose(zt_file);
}
//-MZ.load

void OBJ_DATA::init_set_table()
{
	std::string cppstr, tag;
	int mode = SETSTUFF_SNUM;
	id_to_set_info_map::iterator snum;
	set_info::iterator vnum;
	qty_to_camap_map::iterator oqty;
	class_to_act_map::iterator clss;
	int appnum = 0;

	OBJ_DATA::set_table.clear();

	std::ifstream fp(LIB_MISC "setstuff.lst");

	if (!fp)
	{
		cppstr = "init_set_table:: Unable open input file 'lib/misc/setstuff.lst'";
		mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
		return;
	}

	while (!fp.eof())
	{
		std::istringstream isstream;

		getline(fp, cppstr);

		std::string::size_type i = cppstr.find(';');
		if (i != std::string::npos)
			cppstr.erase(i);
		reverse(cppstr.begin(), cppstr.end());
		isstream.str(cppstr);
		cppstr.erase();
		isstream >> std::skipws >> cppstr;
		if (!isstream.eof())
		{
			std::string suffix;

			getline(isstream, suffix);
			cppstr += suffix;
		}
		reverse(cppstr.begin(), cppstr.end());
		isstream.clear();

		isstream.str(cppstr);
		cppstr.erase();
		isstream >> std::skipws >> cppstr;
		if (!isstream.eof())
		{
			std::string suffix;

			getline(isstream, suffix);
			cppstr += suffix;
		}
		isstream.clear();

		if (cppstr.empty())
			continue;

		if (cppstr[0] == '#')
		{
			if (mode != SETSTUFF_SNUM && mode != SETSTUFF_OQTY && mode != SETSTUFF_AMSG && mode != SETSTUFF_AFFS && mode != SETSTUFF_AFCN)
			{
				cppstr = "init_set_table:: Wrong position of line '" + cppstr + "'";
				mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
				continue;
			}

			cppstr.erase(cppstr.begin());

			if (cppstr.empty() || !a_isdigit(cppstr[0]))
			{
				cppstr = "init_set_table:: Error in line '#" + cppstr + "', expected set id after #";
				mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
				continue;
			}

			int tmpsnum;

			isstream.str(cppstr);
			isstream >> std::noskipws >> tmpsnum;

			if (!isstream.eof())
			{
				cppstr = "init_set_table:: Error in line '#" + cppstr + "', expected only set id after #";
				mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
				continue;
			}

			std::pair< id_to_set_info_map::iterator, bool > p = OBJ_DATA::set_table.insert(std::make_pair(tmpsnum, set_info()));

			if (!p.second)
			{
				cppstr = "init_set_table:: Error in line '#" + cppstr + "', this set already exists";
				mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
				continue;
			}

			snum = p.first;
			mode = SETSTUFF_NAME;
			continue;
		}

		if (cppstr.size() < 5 || cppstr[4] != ':')
		{
			cppstr = "init_set_table:: Format error in line '" + cppstr + "'";
			mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
			continue;
		}

		tag = cppstr.substr(0, 4);
		cppstr.erase(0, 5);
		isstream.str(cppstr);
		cppstr.erase();
		isstream >> std::skipws >> cppstr;
		if (!isstream.eof())
		{
			std::string suffix;

			getline(isstream, suffix);
			cppstr += suffix;
		}
		isstream.clear();

		if (cppstr.empty())
		{
			cppstr = "init_set_table:: Empty parameter field in line '" + tag + ":" + cppstr + "'";
			mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
			continue;
		}

		switch (tag[0])
		{
		case 'A':
			if (tag == "Amsg")
			{
				if (mode != SETSTUFF_AMSG)
				{
					cppstr = "init_set_table:: Wrong position of line '" + tag + ":" + cppstr + "'";
					mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
					continue;
				}

				clss->second.set_actmsg(cppstr);
				mode = SETSTUFF_DMSG;
			}
			else if (tag == "Affs")
			{
				if (mode != SETSTUFF_AMSG && mode != SETSTUFF_AFFS)
				{
					cppstr = "init_set_table:: Wrong position of line '" + tag + ":" + cppstr + "'";
					mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
					continue;
				}

				isstream.str(cppstr);
				cppstr.erase();
				isstream >> std::skipws >> cppstr;
				if (!isstream.eof())
				{
					std::string suffix;

					getline(isstream, suffix);
					cppstr += suffix;
					cppstr = "init_set_table:: Error in line '" + tag + ":" + cppstr + "', expected only object affects";
					mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
					continue;
				}

				FLAG_DATA tmpaffs = clear_flags;
				tmpaffs.from_string(cppstr.c_str());

				clss->second.set_affects(tmpaffs);
				appnum = 0;
				mode = SETSTUFF_AFCN;
			}
			else if (tag == "Afcn")
			{
				if (mode != SETSTUFF_AMSG && mode != SETSTUFF_AFFS && mode != SETSTUFF_AFCN)
				{
					cppstr = "init_set_table:: Wrong position of line '" + tag + ":" + cppstr + "'";
					mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
					continue;
				}

				isstream.str(cppstr);

				int tmploc, tmpmodi;

				if (!(isstream >> std::skipws >> tmploc >> std::skipws >> tmpmodi))
				{
					cppstr = "init_set_table:: Error in line '" + tag + ":" + cppstr + "', expected apply location and modifier";
					mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
					continue;
				}

				obj_affected_type tmpafcn(static_cast<EApplyLocation>(tmploc), (sbyte)tmpmodi);

				if (!isstream.eof())
				{
					cppstr = "init_set_table:: Error in line '" + tag + ":" + cppstr + "', expected only apply location and modifier";
					mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
					continue;
				}

				if (tmpafcn.location <= APPLY_NONE || tmpafcn.location >= NUM_APPLIES)
				{
					cppstr = "init_set_table:: Wrong apply location in line '" + tag + ":" + cppstr + "'";
					mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
					continue;
				}

				if (!tmpafcn.modifier)
				{
					cppstr = "init_set_table:: Wrong apply modifier in line '" + tag + ":" + cppstr + "'";
					mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
					continue;
				}

				if (mode == SETSTUFF_AMSG || mode == SETSTUFF_AFFS)
					appnum = 0;

				if (appnum >= MAX_OBJ_AFFECT)
				{
					cppstr = "init_set_table:: Too many applies - line '" + tag + ":" + cppstr + "'";
					mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
					continue;
				}
				else
					clss->second.set_affected_i(appnum++, tmpafcn);

				mode = SETSTUFF_AFCN;
			}
			else if (tag == "Alis")
			{
				if (mode != SETSTUFF_ALIS)
				{
					cppstr = "init_set_table:: Wrong position of line '" + tag + ":" + cppstr + "'";
					mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
					continue;
				}

				snum->second.set_alias(cppstr);
				mode = SETSTUFF_VNUM;
			}
			else
			{
				cppstr = "init_set_table:: Format error in line '" + tag + ":" + cppstr + "'";
				mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
			}
			break;

		case 'C':
			if (tag == "Clss")
			{
				if (mode != SETSTUFF_CLSS && mode != SETSTUFF_AMSG && mode != SETSTUFF_AFFS && mode != SETSTUFF_AFCN)
				{
					cppstr = "init_set_table:: Wrong position of line '" + tag + ":" + cppstr + "'";
					mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
					continue;
				}

				unique_bit_flag_data tmpclss;

				if (cppstr == "all")
					tmpclss.set_all();
				else
				{
					isstream.str(cppstr);

					int i = 0;

					while (isstream >> std::skipws >> i)
						if (i < 0 || i > NUM_PLAYER_CLASSES * NUM_KIN)
							break;
						else
							tmpclss.set(flag_data_by_num(i));

					if (i < 0 || i > NUM_PLAYER_CLASSES * NUM_KIN)
					{
						cppstr = "init_set_table:: Wrong class in line '" + tag + ":" + cppstr + "'";
						mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
						continue;
					}

					if (!isstream.eof())
					{
						cppstr = "init_set_table:: Error in line '" + tag + ":" + cppstr + "', expected only class ids";
						mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
						continue;
					}
				}

				std::pair< class_to_act_map::iterator, bool > p = oqty->second.insert(std::make_pair(tmpclss, activation()));

				if (!p.second)
				{
					cppstr = "init_set_table:: Error in line '" + tag + ":" + cppstr +
							 "', each class number can occur only once for each object number";
					mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
					continue;
				}

				clss = p.first;
				mode = SETSTUFF_AMSG;
			}
			else
			{
				cppstr = "init_set_table:: Format error in line '" + tag + ":" + cppstr + "'";
				mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
			}
			break;

		case 'D':
			if (tag == "Dmsg")
			{
				if (mode != SETSTUFF_DMSG)
				{
					cppstr = "init_set_table:: Wrong position of line '" + tag + ":" + cppstr + "'";
					mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
					continue;
				}

				clss->second.set_deactmsg(cppstr);
				mode = SETSTUFF_RAMG;
			}
			else if (tag == "Dice")
			{
				if (mode != SETSTUFF_AMSG && mode != SETSTUFF_AFFS && mode != SETSTUFF_AFCN)
				{
					cppstr = "init_set_table:: Wrong position of line '" + tag + ":" + cppstr + "'";
					mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
					continue;
				}

				isstream.str(cppstr);

				int ndices, nsides;

				if (!(isstream >> std::skipws >> ndices >> std::skipws >> nsides))
				{
					cppstr = "init_set_table:: Error in line '" + tag + ":" + cppstr + "', expected ndices and nsides";
					mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
					continue;
				}

				clss->second.set_dices(ndices, nsides);
			}
			else
			{
				cppstr = "init_set_table:: Format error in line '" + tag + ":" + cppstr + "'";
				mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
			}
			break;

		case 'N':
			if (tag == "Name")
			{
				if (mode != SETSTUFF_NAME)
				{
					cppstr = "init_set_table:: Wrong position of line '" + tag + ":" + cppstr + "'";
					mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
					continue;
				}

				snum->second.set_name(cppstr);
				mode = SETSTUFF_ALIS;
			}
			else
			{
				cppstr = "init_set_table:: Format error in line '" + tag + ":" + cppstr + "'";
				mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
			}
			break;

		case 'O':
			if (tag == "Oqty")
			{
				if (mode != SETSTUFF_OQTY && mode != SETSTUFF_AMSG && mode != SETSTUFF_AFFS && mode != SETSTUFF_AFCN)
				{
					cppstr = "init_set_table:: Wrong position of line '" + tag + ":" + cppstr + "'";
					mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
					continue;
				}

				unsigned int tmpoqty = 0;
				isstream.str(cppstr);
				isstream >> std::skipws >> tmpoqty;

				if (!isstream.eof())
				{
					cppstr = "init_set_table:: Error in line '" + tag + ":" + cppstr + "', expected only object number";
					mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
					continue;
				}

				if (!tmpoqty || tmpoqty > NUM_WEARS)
				{
					cppstr = "init_set_table:: Wrong object number in line '" + tag + ":" + cppstr + "'";
					mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
					continue;
				}

				std::pair< qty_to_camap_map::iterator, bool > p = vnum->second.insert(std::make_pair(tmpoqty, class_to_act_map()));

				if (!p.second)
				{
					cppstr = "init_set_table:: Error in line '" + tag + ":" + cppstr +
							 "', each object number can occur only once for each object";
					mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
					continue;
				}

				oqty = p.first;
				mode = SETSTUFF_CLSS;
			}
			else
			{
				cppstr = "init_set_table:: Format error in line '" + tag + ":" + cppstr + "'";
				mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
			}
			break;

		case 'R':
			if (tag == "Ramg")
			{
				if (mode != SETSTUFF_RAMG)
				{
					cppstr = "init_set_table:: Wrong position of line '" + tag + ":" + cppstr + "'";
					mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
					continue;
				}

				clss->second.set_room_actmsg(cppstr);
				mode = SETSTUFF_RDMG;
			}
			else if (tag == "Rdmg")
			{
				if (mode != SETSTUFF_RDMG)
				{
					cppstr = "init_set_table:: Wrong position of line '" + tag + ":" + cppstr + "'";
					mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
					continue;
				}

				clss->second.set_room_deactmsg(cppstr);
				mode = SETSTUFF_AFFS;
			}
			else
			{
				cppstr = "init_set_table:: Format error in line '" + tag + ":" + cppstr + "'";
				mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
			}
			break;

		case 'S':
			if (tag == "Skll")
			{
				if (mode != SETSTUFF_AMSG && mode != SETSTUFF_AFFS && mode != SETSTUFF_AFCN)
				{
					cppstr = "init_set_table:: Wrong position of line '" + tag + ":" + cppstr + "'";
					mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
					continue;
				}

				isstream.str(cppstr);

				int skillnum, percent;

				if (!(isstream >> std::skipws >> skillnum >> std::skipws >> percent))
				{
					cppstr = "init_set_table:: Error in line '" + tag + ":" + cppstr + "', expected ndices and nsides";
					mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
					continue;
				}

				clss->second.set_skill(static_cast<ESkill>(skillnum), percent);
			}
			else
			{
				cppstr = "init_set_table:: Error in line '" + tag + ":" + cppstr + "'";
				mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
			}
			break;

		case 'V':
			if (tag == "Vnum")
			{
				if (mode != SETSTUFF_ALIS
					&& mode != SETSTUFF_VNUM
					&& mode != SETSTUFF_OQTY
					&& mode != SETSTUFF_AMSG
					&& mode != SETSTUFF_AFFS
					&& mode != SETSTUFF_AFCN)
				{
					cppstr = "init_set_table:: Wrong position of line '" + tag + ":" + cppstr + "'";
					mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
					continue;
				}

				obj_vnum tmpvnum = -1;
				isstream.str(cppstr);
				isstream >> std::skipws >> tmpvnum;

				if (!isstream.eof())
				{
					cppstr = "init_set_table:: Error in line '" + tag + ":" + cppstr + "', expected only object vnum";
					mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
					continue;
				}

				if (real_object(tmpvnum) < 0)
				{
					cppstr = "init_set_table:: Wrong object vnum in line '" + tag + ":" + cppstr + "'";
					mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
					continue;
				}

				id_to_set_info_map::iterator it;

				for (it = OBJ_DATA::set_table.begin(); it != OBJ_DATA::set_table.end(); it++)
					if (it->second.find(tmpvnum) != it->second.end())
						break;

				if (it != OBJ_DATA::set_table.end())
				{
					cppstr = "init_set_table:: Error in line '" + tag + ":" + cppstr + "', object can exist only in one set";
					mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
					continue;
				}

				vnum = snum->second.insert(std::make_pair(tmpvnum, qty_to_camap_map())).first;
				mode = SETSTUFF_OQTY;
			}
			else
			{
				cppstr = "init_set_table:: Error in line '" + tag + ":" + cppstr + "'";
				mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
			}
			break;
		case 'W':
			if (tag == "Wght")
			{
				if (mode != SETSTUFF_AMSG && mode != SETSTUFF_AFFS && mode != SETSTUFF_AFCN)
				{
					cppstr = "init_set_table:: Wrong position of line '" + tag + ":" + cppstr + "'";
					mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
					continue;
				}

				isstream.str(cppstr);

				int weight;

				if (!(isstream >> std::skipws >> weight))
				{
					cppstr = "init_set_table:: Error in line '" + tag + ":" + cppstr + "', expected item weight";
					mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
					continue;
				}

				clss->second.set_weight(weight);
			}
			else
			{
				cppstr = "init_set_table:: Error in line '" + tag + ":" + cppstr + "'";
				mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
			}
			break;

		default:
			cppstr = "init_set_table:: Error in line '" + tag + ":" + cppstr + "'";
			mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
		}
	}

	if (mode != SETSTUFF_SNUM && mode != SETSTUFF_OQTY && mode != SETSTUFF_AMSG && mode != SETSTUFF_AFFS && mode != SETSTUFF_AFCN)
	{
		cppstr = "init_set_table:: Last set was deleted, because of unexpected end of file";
		OBJ_DATA::set_table.erase(snum);
		mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
	}
}

void set_zone_mob_level()
{
	for (int i = 0; i <= top_of_zone_table; ++i)
	{
		int level = 0, count = 0;
		for (int nr = 0; nr <= top_of_mobt; ++nr)
		{
			if (mob_index[nr].vnum >= zone_table[i].number * 100
				&& mob_index[nr].vnum <= zone_table[i].number * 100 + 99)
			{
				level += mob_proto[nr].get_level();
				++count;
			}
		}
		zone_table[i].mob_level = count ? level/count : 0;
	}
}

void set_zone_town()
{
	for (int i = 0; i <= top_of_zone_table; ++i)
	{
		zone_table[i].is_town = false;
		int rnum_start = 0, rnum_end = 0;
		if (!get_zone_rooms(i, &rnum_start, &rnum_end))
		{
			continue;
		}
		bool rent_flag = false, bank_flag = false, post_flag = false;
		// ���� ��������� �������, ���� � ��� ���� ������, ������ � ��������
		for (int k = rnum_start; k <= rnum_end; ++k)
		{
			for (CHAR_DATA *ch = world[k]->people; ch; ch = ch->next_in_room)
			{
				if (IS_RENTKEEPER(ch))
				{
					rent_flag = true;
				}
				else if (IS_BANKKEEPER(ch))
				{
					bank_flag = true;
				}
				else if (IS_POSTKEEPER(ch))
				{
					post_flag = true;
				}
			}
		}
		if (rent_flag && bank_flag && post_flag)
		{
			zone_table[i].is_town = true;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
namespace
{

struct snoop_node
{
	// ���� ����
	std::string mail;
	// ������ ����� ����� � ����� �� �����
	std::vector<long> uid_list;
};

// ����� ���� ����� -> ������ ���
typedef std::map<int, std::set<std::string> > ClanMailType;
ClanMailType clan_list;

} // namespace

// * �������� ����������� ����� ������� ����� (�������� ������ � �� ��������).
bool can_snoop(CHAR_DATA *imm, CHAR_DATA *vict)
{
	if (!CLAN(vict))
	{
		return true;
	}

	for (ClanMailType::const_iterator i = clan_list.begin(),
		iend = clan_list.end(); i != iend; ++i)
	{
		std::set<std::string>::const_iterator k = i->second.find(GET_EMAIL(imm));
		if (k != i->second.end() && CLAN(vict)->CheckPolitics(i->first) == POLITICS_WAR)
		{
			return false;
		}
	}
	return true;
}

////////////////////////////////////////////////////////////////////////////////

void load_messages(void)
{
	FILE *fl;
	int i, type;
	struct message_type *messages;
	char chk[128];

	if (!(fl = fopen(MESS_FILE, "r")))
	{
		log("SYSERR: Error reading combat message file %s: %s", MESS_FILE, strerror(errno));
		exit(1);
	}
	for (i = 0; i < MAX_MESSAGES; i++)
	{
		fight_messages[i].a_type = 0;
		fight_messages[i].number_of_attacks = 0;
		fight_messages[i].msg = 0;
	}


	const char* dummyc = fgets(chk, 128, fl);
	while (!feof(fl) && (*chk == '\n' || *chk == '*'))
	{
		dummyc = fgets(chk, 128, fl);
	}

	while (*chk == 'M')
	{
		dummyc = fgets(chk, 128, fl);

		int dummyi = sscanf(chk, " %d\n", &type);
		UNUSED_ARG(dummyi);

		for (i = 0; (i < MAX_MESSAGES) &&
				(fight_messages[i].a_type != type) && (fight_messages[i].a_type); i++);
		if (i >= MAX_MESSAGES)
		{
			log("SYSERR: Too many combat messages.  Increase MAX_MESSAGES and recompile.");
			exit(1);
		}
		log("BATTLE MESSAGE %d(%d)", i, type);
		CREATE(messages, 1);
		fight_messages[i].number_of_attacks++;
		fight_messages[i].a_type = type;
		messages->next = fight_messages[i].msg;
		fight_messages[i].msg = messages;

		messages->die_msg.attacker_msg = fread_action(fl, i);
		messages->die_msg.victim_msg = fread_action(fl, i);
		messages->die_msg.room_msg = fread_action(fl, i);
		messages->miss_msg.attacker_msg = fread_action(fl, i);
		messages->miss_msg.victim_msg = fread_action(fl, i);
		messages->miss_msg.room_msg = fread_action(fl, i);
		messages->hit_msg.attacker_msg = fread_action(fl, i);
		messages->hit_msg.victim_msg = fread_action(fl, i);
		messages->hit_msg.room_msg = fread_action(fl, i);
		messages->god_msg.attacker_msg = fread_action(fl, i);
		messages->god_msg.victim_msg = fread_action(fl, i);
		messages->god_msg.room_msg = fread_action(fl, i);
		dummyc = fgets(chk, 128, fl);
		while (!feof(fl) && (*chk == '\n' || *chk == '*'))
		{
			dummyc = fgets(chk, 128, fl);
		}
	}
	UNUSED_ARG(dummyc);

	fclose(fl);
}

// body of the booting system
void boot_db(void)
{
	utils::CSteppedProfiler boot_profiler("MUD booting");

	log("Boot db -- BEGIN.");

	boot_profiler.next_step("Creating directories");
	struct stat st;
	#define MKDIR(dir) if (stat((dir), &st) != 0) \
		mkdir((dir), 0744)
	#define MKLETTERS(BASE) MKDIR(#BASE); \
		MKDIR(#BASE "/A-E"); \
		MKDIR(#BASE "/F-J"); \
		MKDIR(#BASE "/K-O"); \
		MKDIR(#BASE "/P-T"); \
		MKDIR(#BASE "/U-Z"); \
		MKDIR(#BASE "/ZZZ")

	MKDIR("etc");
	MKDIR("etc/board");
	MKLETTERS(plralias);
	MKLETTERS(plrobjs);
	MKLETTERS(plrs);
	MKLETTERS(plrvars);
	MKDIR("plrstuff");
	MKDIR("plrstuff/depot");
	MKLETTERS(plrstuff/depot);
	MKDIR("plrstuff/house");
	MKDIR("stat");
	
	#undef MKLETTERS
	#undef MKDIR

	boot_profiler.next_step("Initialization of text IDs");
	log("Init TextId list.");
	TextId::init();

	boot_profiler.next_step("Resetting the game time");
	log("Resetting the game time:");
	reset_time();

	boot_profiler.next_step("Reading credits, help, bground, info & motds.");
	log("Reading credits, help, bground, info & motds.");
	file_to_string_alloc(CREDITS_FILE, &credits);
	file_to_string_alloc(MOTD_FILE, &motd);
	file_to_string_alloc(RULES_FILE, &rules);
	file_to_string_alloc(HELP_PAGE_FILE, &help);
	file_to_string_alloc(INFO_FILE, &info);
	file_to_string_alloc(IMMLIST_FILE, &immlist);
	file_to_string_alloc(POLICIES_FILE, &policies);
	file_to_string_alloc(HANDBOOK_FILE, &handbook);
	file_to_string_alloc(BACKGROUND_FILE, &background);
	file_to_string_alloc(NAME_RULES_FILE, &name_rules);
	if (file_to_string_alloc(GREETINGS_FILE, &GREETINGS) == 0)
		prune_crlf(GREETINGS);

	boot_profiler.next_step("Loading new skills definitions");
    log("Loading NEW skills definitions");
	pugi::xml_document doc;

	Skill::Load(XMLLoad(LIB_MISC SKILLS_FILE, SKILLS_MAIN_TAG, SKILLS_ERROR_STR, doc));
	
	boot_profiler.next_step("Loading criterion");
	log("Loading Criterion...");
	pugi::xml_document doc1;
	Load_Criterion(XMLLoad(LIB_MISC CRITERION_FILE, "finger", "Error Loading Criterion.xml: <finger>", doc1), EWearFlag::ITEM_WEAR_FINGER);
	Load_Criterion(XMLLoad(LIB_MISC CRITERION_FILE, "neck", "Error Loading Criterion.xml: <neck>", doc1), EWearFlag::ITEM_WEAR_NECK);
	Load_Criterion(XMLLoad(LIB_MISC CRITERION_FILE, "body", "Error Loading Criterion.xml: <body>", doc1), EWearFlag::ITEM_WEAR_BODY);
	Load_Criterion(XMLLoad(LIB_MISC CRITERION_FILE, "head", "Error Loading Criterion.xml: <head>", doc1), EWearFlag::ITEM_WEAR_HEAD);
	Load_Criterion(XMLLoad(LIB_MISC CRITERION_FILE, "legs", "Error Loading Criterion.xml: <legs>", doc1), EWearFlag::ITEM_WEAR_LEGS);
	Load_Criterion(XMLLoad(LIB_MISC CRITERION_FILE, "feet", "Error Loading Criterion.xml: <feet>", doc1), EWearFlag::ITEM_WEAR_FEET);
	Load_Criterion(XMLLoad(LIB_MISC CRITERION_FILE, "hands", "Error Loading Criterion.xml: <hands>", doc1), EWearFlag::ITEM_WEAR_HANDS);
	Load_Criterion(XMLLoad(LIB_MISC CRITERION_FILE, "arms", "Error Loading Criterion.xml: <arms>", doc1), EWearFlag::ITEM_WEAR_ARMS);
	Load_Criterion(XMLLoad(LIB_MISC CRITERION_FILE, "shield", "Error Loading Criterion.xml: <shield>", doc1), EWearFlag::ITEM_WEAR_SHIELD);
	Load_Criterion(XMLLoad(LIB_MISC CRITERION_FILE, "about", "Error Loading Criterion.xml: <about>", doc1), EWearFlag::ITEM_WEAR_ABOUT);
	Load_Criterion(XMLLoad(LIB_MISC CRITERION_FILE, "waist", "Error Loading Criterion.xml: <waist>", doc1), EWearFlag::ITEM_WEAR_WAIST);
	Load_Criterion(XMLLoad(LIB_MISC CRITERION_FILE, "wrist", "Error Loading Criterion.xml: <wrist>", doc1), EWearFlag::ITEM_WEAR_WRIST);
	Load_Criterion(XMLLoad(LIB_MISC CRITERION_FILE, "wield", "Error Loading Criterion.xml: <wield>", doc1), EWearFlag::ITEM_WEAR_WIELD);
	Load_Criterion(XMLLoad(LIB_MISC CRITERION_FILE, "hold", "Error Loading Criterion.xml: <hold>", doc1), EWearFlag::ITEM_WEAR_HOLD);
	Load_Criterion(XMLLoad(LIB_MISC CRITERION_FILE, "boths", "Error Loading Criterion.xml: <boths>", doc1), EWearFlag::ITEM_WEAR_BOTHS);

	boot_profiler.next_step("Loading birth places definitions");
	log("Loading birth places definitions.");
	BirthPlace::Load(XMLLoad(LIB_MISC BIRTH_PLACES_FILE, BIRTH_PLACE_MAIN_TAG, BIRTH_PLACE_ERROR_STR, doc));

	boot_profiler.next_step("Loading player races definitions");
    log("Loading player races definitions.");
	PlayerRace::Load(XMLLoad(LIB_MISC PLAYER_RACE_FILE, RACE_MAIN_TAG, PLAYER_RACE_ERROR_STR, doc));

	boot_profiler.next_step("Loading spell definitions");
	log("Loading spell definitions.");
	mag_assign_spells();

	boot_profiler.next_step("Loading feature definitions");
	log("Loading feature definitions.");
	assign_feats();

	boot_profiler.next_step("Loading ingredients magic");
	log("Booting IM");
	init_im();

	boot_profiler.next_step("Loading zone types and ingredient for each zone type");
	log("Booting zone types and ingredient types for each zone type.");
	init_zone_types();

	boot_profiler.next_step("Loading insert_wanted.lst");
	log("Booting insert_wanted.lst.");
	iwg.init();

	boot_profiler.next_step("Loading gurdians");
	log("Load guardians.");
	load_guardians();

	boot_profiler.next_step("Loading world");
	world_loader.boot_world();

	boot_profiler.next_step("Loading stuff load table");
	log("Booting stuff load table.");
	oload_table.init();

	boot_profiler.next_step("Loading setstuff table");
	log("Booting setstuff table.");
	OBJ_DATA::init_set_table();

	boot_profiler.next_step("Loading item levels");
	log("Init item levels.");
	ObjSystem::init_item_levels();

	boot_profiler.next_step("Loading help entries");
	log("Loading help entries.");
	world_loader.index_boot(DB_BOOT_HLP);

	boot_profiler.next_step("Loading social entries");
	log("Loading social entries.");
	world_loader.index_boot(DB_BOOT_SOCIAL);

	boot_profiler.next_step("Loading players index");
	log("Generating player index.");
	build_player_index();

	// ���� ������ ����� ��������� �����-�������
	boot_profiler.next_step("Loading CRC system");
	log("Loading file crc system.");
	FileCRC::load();

	boot_profiler.next_step("Loading fight messages");
	log("Loading fight messages.");
	load_messages();

	boot_profiler.next_step("Assigning function pointers");
	log("Assigning function pointers:");
	if (!no_specials)
	{
		log("   Mobiles.");
		assign_mobiles();
		log("   Objects.");
		assign_objects();
		log("   Rooms.");
		assign_rooms();
	}

	boot_profiler.next_step("Assigning spells and skills levels");
	log("Assigning spell and skill levels.");
	init_spell_levels();

	boot_profiler.next_step("Sorting command list");
	log("Sorting command list.");
	sort_commands();

	boot_profiler.next_step("Reading banned site, proxy, privileges and invalid-name list.");
	log("Reading banned site, proxy, privileges and invalid-name list.");
	ban = new BanList();
	Read_Invalid_List();

	boot_profiler.next_step("Loading rented objects info");
	log("Booting rented objects info");
	zone_rnum i;
	for (i = 0; i <= top_of_p_table; i++)
	{
		(player_table + i)->timer = NULL;
		Crash_read_timer(i, FALSE);
	}

	// ������������������ ����� ������/����� �� ������
	boot_profiler.next_step("Loading boards");
	log("Booting boards");
	Boards::Static::BoardInit();

	boot_profiler.next_step("loading clans");
	log("Booting clans");
	Clan::ClanLoad();

	// �������� ������ ������� �����
	boot_profiler.next_step("Loading named stuff");
	log("Load named stuff");
	NamedStuff::load();

	boot_profiler.next_step("Loading basic values");
	log("Booting basic values");
	init_basic_values();

	boot_profiler.next_step("Loading grouping parameters");
	log("Booting grouping parameters");
	if (grouping.init())
	{
		exit(1);
	}

	boot_profiler.next_step("Loading special assignments");
	log("Booting special assignment");
	init_spec_procs();

	boot_profiler.next_step("Loading guilds");
	log("Booting guilds");
	init_guilds();

	boot_profiler.next_step("Loading portals for 'town portal' spell");
	log("Booting portals for 'town portal' spell");
	portals_list = NULL;
	init_portals();

	boot_profiler.next_step("Loading made items");
	log("Booting maked items");
	init_make_items();

	boot_profiler.next_step("Loading exchange");
	log("Booting exchange");
	exchange_database_load();

	boot_profiler.next_step("Loading scheduled reboot time");
	log("Load schedule reboot time");
	load_sheduled_reboot();

	boot_profiler.next_step("Loading proxies list");
	log("Load proxy list");
	LoadProxyList();

	boot_profiler.next_step("Loading new_name list");
	log("Load new_name list");
	NewNameLoad();

	boot_profiler.next_step("Loading global UID timer");
	log("Load global uid counter");
	LoadGlobalUID();

	boot_profiler.next_step("Initializing DT list");
	log("Init DeathTrap list.");
	DeathTrap::load();

	boot_profiler.next_step("Loading titles list");
	log("Load Title list.");
	TitleSystem::load_title_list();

	boot_profiler.next_step("Loading emails list");
	log("Load registered emails list.");
	RegisterSystem::load();

	boot_profiler.next_step("Loading privileges and gods list");
	log("Load privilege and god list.");
	Privilege::load();

	// ������ ���� �� ������ ���
	boot_profiler.next_step("Initializing depot system");
	log("Init Depot system.");
	Depot::init_depot();

	boot_profiler.next_step("Loading Parcel system");
	log("Init Parcel system.");
	Parcel::load();

	boot_profiler.next_step("Loading celebrates");
	log("Load Celebrates."); //Polud ���������. ������������ ��� ������ ���
	//Celebrates::load(XMLLoad(LIB_MISC CELEBRATES_FILE, CELEBRATES_MAIN_TAG, CELEBRATES_ERROR_STR));
	Celebrates::load();

	// ����� ������ ���� ����� ����� ���� ������ ��� ��� (��������� � �.�.)
	boot_profiler.next_step("Resetting zones");
	for (i = 0; i <= top_of_zone_table; i++)
	{
		log("Resetting %s (rooms %d-%d).", zone_table[i].name,
			(i ? (zone_table[i - 1].top + 1) : 0), zone_table[i].top);
		reset_zone(i);
	}
	reset_q.head = reset_q.tail = NULL;

	// �������� ����� ������ ���, �� ������ � �������
	boot_profiler.next_step("Loading depot chests");
	log("Load depot chests.");
	Depot::load_chests();

	boot_profiler.next_step("Loading glory");
	log("Load glory.");
	Glory::load_glory();
	log("Load const glory.");
	GloryConst::load();
	log("Load glory log.");
	GloryMisc::load_log();

	//Polud ������ ��������� ��� �����
	boot_profiler.next_step("Loading mob races");
	log("Load mob races.");
	load_mobraces();

	boot_profiler.next_step("Loading morphs");
	log("Load morphs.");
	load_morphs();

	boot_profiler.next_step("Initializing global drop list");
	log("Init global drop list.");
	GlobalDrop::init();

	boot_profiler.next_step("Loading offtop block list");
	log("Init offtop block list.");
	OfftopSystem::init();

	boot_profiler.next_step("Loading shop_ext list");
	log("load shop_ext list start.");
	ShopExt::load(false);
	log("load shop_ext list stop.");

	boot_profiler.next_step("Loading zone average mob_level");
	log("Set zone average mob_level.");
	set_zone_mob_level();

	boot_profiler.next_step("Setting zone town");
	log("Set zone town.");
	set_zone_town();

	boot_profiler.next_step("Initializing town shop_keepers");
	log("Init town shop_keepers.");
	town_shop_keepers();

	/* Commented out until full implementation
	boot_profiler.next_step("Loading craft system");
	log("Starting craft system.");
	if (!craft::start())
	{
		log("ERROR: Failed to start craft system.\n");
	}
	*/

	boot_profiler.next_step("Loading big sets in rent");
	log("Check big sets in rent.");
	SetSystem::check_rented();

	// ������� ����, ����� �����, ����� ���� �����
	boot_profiler.next_step("Loading object sets/mob_stat/drop_sets lists");
	obj_sets::load();
	log("Load mob_stat.xml");
	mob_stat::load();
	log("Init SetsDrop lists.");
	SetsDrop::init();

	boot_profiler.next_step("Loading remorts");
	log("Load remort.xml");
	Remort::init();

	boot_profiler.next_step("Loading noob_help.xml");
	log("Load noob_help.xml");
	Noob::init();

	boot_profiler.next_step("Loading reset_stats.xml");
	log("Load reset_stats.xml");
	ResetStats::init();

	boot_profiler.next_step("Loading mail.xml");
	log("Load mail.xml");
	mail::load();
	
	// �������� ������
	boot_profiler.next_step("Loading cases");
	load_cases();
	
	// ������� ������ ������� ����� ����� ����, ��� ����� � ��� ���-�� ��������
	boot_profiler.next_step("Reloading help system");
	HelpSystem::reload_all();

	boot_profiler.next_step("Loading bonus log");
	Bonus::bonus_log_load();
	load_speedwalk();

	boot_profiler.next_step("Loading class_limit.xml");
	log("Load class_limit.xml");
	load_class_limit();
	load_cities();
	shutdown_parameters.mark_boot_time();
	log("Boot db -- DONE.");
	
}

// reset the time in the game from file
void reset_time(void)
{
	time_info = *mud_time_passed(time(0), beginning_of_time);
	// Calculate moon day
	weather_info.moon_day =
		((time_info.year * MONTHS_PER_YEAR + time_info.month) * DAYS_PER_MONTH + time_info.day) % MOON_CYCLE;
	weather_info.week_day_mono =
		((time_info.year * MONTHS_PER_YEAR + time_info.month) * DAYS_PER_MONTH + time_info.day) % WEEK_CYCLE;
	weather_info.week_day_poly =
		((time_info.year * MONTHS_PER_YEAR + time_info.month) * DAYS_PER_MONTH + time_info.day) % POLY_WEEK_CYCLE;
	// Calculate Easter
	calc_easter();

	if (time_info.hours < sunrise[time_info.month][0])
		weather_info.sunlight = SUN_DARK;
	else if (time_info.hours == sunrise[time_info.month][0])
		weather_info.sunlight = SUN_RISE;
	else if (time_info.hours < sunrise[time_info.month][1])
		weather_info.sunlight = SUN_LIGHT;
	else if (time_info.hours == sunrise[time_info.month][1])
		weather_info.sunlight = SUN_SET;
	else
		weather_info.sunlight = SUN_DARK;

	log("   Current Gametime: %dH %dD %dM %dY.", time_info.hours, time_info.day, time_info.month, time_info.year);

	weather_info.temperature = (year_temp[time_info.month].med * 4 +
								number(year_temp[time_info.month].min, year_temp[time_info.month].max)) / 5;
	weather_info.pressure = 960;
	if ((time_info.month >= MONTH_MAY) && (time_info.month <= MONTH_NOVEMBER))
		weather_info.pressure += dice(1, 50);
	else
		weather_info.pressure += dice(1, 80);

	weather_info.change = 0;
	weather_info.weather_type = 0;

	if (time_info.month >= MONTH_APRIL && time_info.month <= MONTH_MAY)
		weather_info.season = SEASON_SPRING;
	else if (time_info.month == MONTH_MART && weather_info.temperature >= 3)
		weather_info.season = SEASON_SPRING;
	else if (time_info.month >= MONTH_JUNE && time_info.month <= MONTH_AUGUST)
		weather_info.season = SEASON_SUMMER;
	else if (time_info.month >= MONTH_SEPTEMBER && time_info.month <= MONTH_OCTOBER)
		weather_info.season = SEASON_AUTUMN;
	else if (time_info.month == MONTH_NOVEMBER && weather_info.temperature >= 3)
		weather_info.season = SEASON_AUTUMN;
	else
		weather_info.season = SEASON_WINTER;

	if (weather_info.pressure <= 980)
		weather_info.sky = SKY_LIGHTNING;
	else if (weather_info.pressure <= 1000)
	{
		weather_info.sky = SKY_RAINING;
		if (time_info.month >= MONTH_APRIL && time_info.month <= MONTH_OCTOBER)
			create_rainsnow(&weather_info.weather_type, WEATHER_LIGHTRAIN, 40, 40, 20);
		else if (time_info.month >= MONTH_DECEMBER || time_info.month <= MONTH_FEBRUARY)
			create_rainsnow(&weather_info.weather_type, WEATHER_LIGHTSNOW, 50, 40, 10);
		else if (time_info.month == MONTH_NOVEMBER || time_info.month == MONTH_MART)
		{
			if (weather_info.temperature >= 3)
				create_rainsnow(&weather_info.weather_type, WEATHER_LIGHTRAIN, 70, 30, 0);
			else
				create_rainsnow(&weather_info.weather_type, WEATHER_LIGHTSNOW, 80, 20, 0);
		}
	}
	else if (weather_info.pressure <= 1020)
		weather_info.sky = SKY_CLOUDY;
	else
		weather_info.sky = SKY_CLOUDLESS;
}

// generate index table for the player file
void build_player_index(void)
{
	new_build_player_index();
	return;
}

void GameLoader::index_boot(const EBootType mode)
{
	log("Index booting %d", mode);

	auto index = IndexFileFactory::get_index(mode);
	if (!index->open())
	{
		exit(1);
	}
	const int rec_count = index->load();

	prepare_global_structures(mode, rec_count);

	for (const auto& entry: *index)
	{
		auto data_file = DataFileFactory::get_file(mode, entry);
		if (!data_file->open())
		{
			continue;	// TODO: we need to react somehow.
		}
		if (!data_file->load())
		{
			// TODO: do something
		}
		data_file->close();
	}

	// sort the social index
	if (mode == DB_BOOT_SOCIAL)
	{
		qsort(soc_keys_list, number_of_social_commands, sizeof(struct social_keyword), csort);
	}
}

void GameLoader::prepare_global_structures(const EBootType mode, const int rec_count)
{
	// * NOTE: "bytes" does _not_ include strings or other later malloc'd things.
	switch (mode)
	{
	case DB_BOOT_TRG:
		CREATE(trig_index, rec_count);
		break;

	case DB_BOOT_WLD:
		{
			// Creating empty world with NOWHERE room.
			world.push_back(new ROOM_DATA);
			top_of_world = FIRST_ROOM;
			const size_t rooms_bytes = sizeof(ROOM_DATA) * rec_count;
			log("   %d rooms, %zd bytes.", rec_count, rooms_bytes);
		}
		break;

	case DB_BOOT_MOB:
		{
			mob_proto = new CHAR_DATA[rec_count]; // TODO: ��������� �� ������ (+� medit)
			CREATE(mob_index, rec_count);
			const size_t index_size = sizeof(INDEX_DATA) * rec_count;
			const size_t characters_size = sizeof(CHAR_DATA) * rec_count;
			log("   %d mobs, %zd bytes in index, %zd bytes in prototypes.", rec_count, index_size, characters_size);
		}
		break;

	case DB_BOOT_OBJ:
		log("   %d objs, ~%zd bytes in index, ~%zd bytes in prototypes.", rec_count, obj_proto.index_size(), obj_proto.prototypes_size());
		break;

	case DB_BOOT_ZON:
		{
			CREATE(zone_table, rec_count);
			const size_t zones_size = sizeof(struct zone_data) * rec_count;
			log("   %d zones, %zd bytes.", rec_count, zones_size);
		}
		break;

	case DB_BOOT_HLP:
		break;

	case DB_BOOT_SOCIAL:
		{
			CREATE(soc_mess_list, number_of_social_messages);
			CREATE(soc_keys_list, number_of_social_commands);
			const size_t messages_size = sizeof(struct social_messg) * (number_of_social_messages);
			const size_t keywords_size = sizeof(struct social_keyword) * (number_of_social_commands);
			log("   %d entries(%d keywords), %zd(%zd) bytes.", number_of_social_messages,
				number_of_social_commands, messages_size, keywords_size);
		}
		break;
	}
}

// * �������� ������ ������������ ������ �� ��������.
void check_room_flags(int rnum)
{
	if (DeathTrap::is_slow_dt(rnum))
	{
		// ������ ������� � ������ ������, ����������� ���� ��������� �� ������� ��� ������� ��� ������� ���������� ��
		GET_ROOM(rnum)->unset_flag(ROOM_NOMAGIC);
		GET_ROOM(rnum)->unset_flag(ROOM_NOTELEPORTOUT);
		GET_ROOM(rnum)->unset_flag(ROOM_NOSUMMON);
	}
	if (GET_ROOM(rnum)->get_flag(ROOM_HOUSE)
		&& (SECT(rnum) == SECT_MOUNTAIN || SECT(rnum) == SECT_HILLS))
	{
		// ��� � ������ ����� �� ������
		SECT(rnum) = SECT_INSIDE;
	}
}

// make sure the start rooms exist & resolve their vnums to rnums
void check_start_rooms(void)
{
	if ((r_mortal_start_room = real_room(mortal_start_room)) == NOWHERE)
	{
		log("SYSERR:  Mortal start room does not exist.  Change in config.c. %d", mortal_start_room);
		exit(1);
	}

	if ((r_immort_start_room = real_room(immort_start_room)) == NOWHERE)
	{
		log("SYSERR:  Warning: Immort start room does not exist.  Change in config.c.");
		r_immort_start_room = r_mortal_start_room;
	}

	if ((r_frozen_start_room = real_room(frozen_start_room)) == NOWHERE)
	{
		log("SYSERR:  Warning: Frozen start room does not exist.  Change in config.c.");
		r_frozen_start_room = r_mortal_start_room;
	}

	if ((r_helled_start_room = real_room(helled_start_room)) == NOWHERE)
	{
		log("SYSERR:  Warning: Hell start room does not exist.  Change in config.c.");
		r_helled_start_room = r_mortal_start_room;
	}

	if ((r_named_start_room = real_room(named_start_room)) == NOWHERE)
	{
		log("SYSERR:  Warning: NAME start room does not exist.  Change in config.c.");
		r_named_start_room = r_mortal_start_room;
	}

	if ((r_unreg_start_room = real_room(unreg_start_room)) == NOWHERE)
	{
		log("SYSERR:  Warning: UNREG start room does not exist.  Change in config.c.");
		r_unreg_start_room = r_mortal_start_room;
	}
}

// resolve all vnums into rnums in the world
void renum_world(void)
{
	int room, door;

	for (room = FIRST_ROOM; room <= top_of_world; room++)
		for (door = 0; door < NUM_OF_DIRS; door++)
			if (world[room]->dir_option[door])
				if (world[room]->dir_option[door]->to_room != NOWHERE)
					world[room]->dir_option[door]->to_room =
						real_room(world[room]->dir_option[door]->to_room);
}

// ��������� �������������� � ���� � ����������
void renum_obj_zone(void)
{
	for (size_t i = 0; i < obj_proto.size(); ++i)
	{
		obj_proto.zone(i, real_zone(obj_proto[i]->get_vnum()));
	}
}

// ����������������������� � ���� � �������
void renum_mob_zone(void)
{
	int i;
	for (i = 0; i <= top_of_mobt; ++i)
	{
		mob_index[i].zone = real_zone(mob_index[i].vnum);
	}
}

#define ZCMD_CMD(cmd_nom) zone_table[zone].cmd[cmd_nom]
#define ZCMD zone_table[zone].cmd[cmd_no]

void renum_single_table(int zone)
{
	int cmd_no, a, b, c, olda, oldb, oldc;
	char buf[128];

	for (cmd_no = 0; ZCMD.command != 'S'; cmd_no++)
	{
		a = b = c = 0;
		olda = ZCMD.arg1;
		oldb = ZCMD.arg2;
		oldc = ZCMD.arg3;
		switch (ZCMD.command)
		{
		case 'M':
			a = ZCMD.arg1 = real_mobile(ZCMD.arg1);
			c = ZCMD.arg3 = real_room(ZCMD.arg3);
			break;
		case 'F':
			a = ZCMD.arg1 = real_room(ZCMD.arg1);
			b = ZCMD.arg2 = real_mobile(ZCMD.arg2);
			c = ZCMD.arg3 = real_mobile(ZCMD.arg3);
			break;
		case 'O':
			a = ZCMD.arg1 = real_object(ZCMD.arg1);
			if (ZCMD.arg3 != NOWHERE)
				c = ZCMD.arg3 = real_room(ZCMD.arg3);
			break;
		case 'G':
			a = ZCMD.arg1 = real_object(ZCMD.arg1);
			break;
		case 'E':
			a = ZCMD.arg1 = real_object(ZCMD.arg1);
			break;
		case 'P':
			a = ZCMD.arg1 = real_object(ZCMD.arg1);
			c = ZCMD.arg3 = real_object(ZCMD.arg3);
			break;
		case 'D':
			a = ZCMD.arg1 = real_room(ZCMD.arg1);
			break;
		case 'R':	// rem obj from room
			a = ZCMD.arg1 = real_room(ZCMD.arg1);
			b = ZCMD.arg2 = real_object(ZCMD.arg2);
			break;
		case 'Q':
			a = ZCMD.arg1 = real_mobile(ZCMD.arg1);
			break;
		case 'T':	// a trigger
			// designer's choice: convert this later
			// b = ZCMD.arg2 = real_trigger(ZCMD.arg2);
			b = real_trigger(ZCMD.arg2);	// leave this in for validation
			if (ZCMD.arg1 == WLD_TRIGGER)
				c = ZCMD.arg3 = real_room(ZCMD.arg3);
			break;
		case 'V':	// trigger variable assignment
			if (ZCMD.arg1 == WLD_TRIGGER)
				b = ZCMD.arg2 = real_room(ZCMD.arg2);
			break;
		}
		if (a < 0 || b < 0 || c < 0)
		{
			sprintf(buf, "Invalid vnum %d, cmd disabled", (a < 0) ? olda : ((b < 0) ? oldb : oldc));
			log_zone_error(zone, cmd_no, buf);
			ZCMD.command = '*';
		}
	}
}

// resove vnums into rnums in the zone reset tables
void renum_zone_table(void)
{
	zone_rnum zone;
	for (zone = 0; zone <= top_of_zone_table; zone++)
	{
		renum_single_table(zone);
	}
}

/*
 * interpret_espec is the function that takes espec keywords and values
 * and assigns the correct value to the mob as appropriate.  Adding new
 * e-specs is absurdly easy -- just add a new CASE statement to this
 * function!  No other changes need to be made anywhere in the code.
 */

// Make own name by process aliases
int trans_obj_name(OBJ_DATA * obj, CHAR_DATA * ch)
{
	// ���� ����� @p , @p1 ... � �������� �� ������.
	int i, k;
	for (i = 0; i < CObjectPrototype::NUM_PADS; i++)
	{
		std::string obj_pad = GET_OBJ_PNAME(obj_proto[GET_OBJ_RNUM(obj)], i);
		size_t j = obj_pad.find("@p");
		if (std::string::npos != j && 0 < j)
		{
			// �������� ������ ����������� ���.
			k = atoi(obj_pad.substr(j + 2, j + 3).c_str());
			obj_pad.replace(j, 3, GET_PAD(ch, k));

			obj->set_PName(i, obj_pad);
			// ���� ��� � ������������ �� ��������� ������
			if (i == 0)
			{
				obj->set_short_description(obj_pad);
				obj->set_aliases(obj_pad); // ������ ������
			}
		}
	};
	obj->set_is_rename(true); // �������� ������ ��� � ������ �������� ������
	return (TRUE);
}

void dl_list_copy(load_list * *pdst, load_list * src)
{
	load_list::iterator p;
	if (src == NULL)
	{
		*pdst = NULL;
		return;
	}
	else
	{
		*pdst = new load_list;
		p = src->begin();
		while (p != src->end())
		{
			(*pdst)->push_back(*p);
			p++;
		}
	}
}


// Dead load list object load
int dl_load_obj(OBJ_DATA * corpse, CHAR_DATA * ch, CHAR_DATA * chr, int DL_LOAD_TYPE)
{
	bool last_load = true;
	bool load = false;
	bool miw;
	load_list::iterator p;

	if (mob_proto[GET_MOB_RNUM(ch)].dl_list == NULL)
		return FALSE;

	p = mob_proto[GET_MOB_RNUM(ch)].dl_list->begin();

	while (p != mob_proto[GET_MOB_RNUM(ch)].dl_list->end())
	{
		switch ((*p)->load_type)
		{
		case DL_LOAD_ANYWAY:
			last_load = load;
		case DL_LOAD_ANYWAY_NC:
			break;
		case DL_LOAD_IFLAST:
			last_load = load && last_load;
		case DL_LOAD_IFLAST_NC:
			load = load && last_load;
			break;
		}
		// ��������� ���� � ����������� �� �������� �������������
		if ((*p)->spec_param != DL_LOAD_TYPE)
			load = false;
		else
			load = true;
		if (load)
		{
			const auto tobj = world_objects.create_from_prototype_by_vnum((*p)->obj_vnum);
			if (!tobj)
			{
				sprintf(buf, "������� �������� � ���� (VNUM:%d) ��������������� ������� (VNUM:%d).",
					GET_MOB_VNUM(ch), (*p)->obj_vnum);
				mudlog(buf, NRM, LVL_BUILDER, ERRLOG, TRUE);
			}
			else
			{
				// ��������� ���_��_����� � ����������� ��������, ���� ��� ���������� ��� ������ DL_LOAD_TYPE
				if (GET_OBJ_MIW(tobj) >= obj_proto.actual_count(tobj->get_rnum())
					|| GET_OBJ_MIW(tobj) == OBJ_DATA::UNLIMITED_GLOBAL_MAXIMUM
					|| check_unlimited_timer(tobj.get()))
				{
					miw = true;
				}
				else
				{
					miw = false;
				}

				switch (DL_LOAD_TYPE)
				{
				case DL_ORDINARY:	//������� �������� - ��� ����������
					if (miw && (number(1, 100) <= (*p)->load_prob))
						load = true;
					else
						load = false;
					break;

				case DL_PROGRESSION:	//�������� � ��������� �� 0.01 ������������
					if ((miw && (number(1, 100) <= (*p)->load_prob)) || (number(1, 100) <= 1))
						load = true;
					else
						load = false;
					break;

				case DL_SKIN:	//�������� �� ���������� "����������"
					if ((miw && (number(1, 100) <= (*p)->load_prob)) || (number(1, 100) <= 1))
						load = true;
					else
						load = false;
					if (chr == NULL)
						load = false;
					break;
				}
				if (load)
				{
					tobj->set_zone(world[ch->in_room]->zone);
					tobj->set_parent(GET_MOB_VNUM(ch));
					if (DL_LOAD_TYPE == DL_SKIN)
					{
						trans_obj_name(tobj.get(), ch);
					}
					// ��������� �������� �� ���������� �����
					if (MOB_FLAGGED(ch, MOB_CORPSE))
					{
						act("�� ����� �����$U ������ $o.", FALSE, ch, tobj.get(), 0, TO_ROOM);
						obj_to_room(tobj.get(), ch->in_room);
					}
					else
					{
						if ((DL_LOAD_TYPE == DL_SKIN) && (corpse->get_carried_by() == chr))
						{
							can_carry_obj(chr, tobj.get());
						}
						else
						{
							obj_to_obj(tobj.get(), corpse);
						}
					}
				}
				else
				{
					extract_obj(tobj.get());
					load = false;
				}

			}
		}
		p++;
	}

	return TRUE;
}

// Dead load list object parse
int dl_parse(load_list ** dl_list, char *line)
{
	// ������ �������� D {����� ���������} {����������� ��������} {���� ���� - ��� ��������}
	int vnum, prob, type, spec;
	struct load_data *new_item;

	if (sscanf(line, "%d %d %d %d", &vnum, &prob, &type, &spec) != 4)
	{
		// ������ ������.
		log("SYSERR: Parse death load list (bad param count).");
		return FALSE;
	};
	// ��������� ������������� ��������� � ���� (�������� ��� ������ ���� ���������)
	if (*dl_list == NULL)
	{
		// ������� ����� ������.
		*dl_list = new load_list;
	}

	CREATE(new_item, 1);
	new_item->obj_vnum = vnum;
	new_item->load_prob = prob;
	new_item->load_type = type;
	new_item->spec_param = spec;

	(*dl_list)->push_back(new_item);
	return TRUE;
}

namespace {

int test_levels[] = {
	100,
	300,
	600,
	1000,
	1500,
	2100,
	2900,
	3900,
	5100,
	6500, // 10
	8100,
	10100,
	12500,
	15300,
	18500,
	22500,
	27300,
	32900,
	39300,
	46500, // 20
	54500,
	64100,
	75300,
	88100,
	102500,
	117500,
	147500,
	192500,
	252500,
	327500, // 30
	417500,
	522500,
	642500,
	777500,
	927500,
	1127500,
	1427500,
	1827500,
	2327500,
	2927500, // 40
	3627500,
	4427500,
	5327500,
	6327500,
	7427500,
	8627500,
	9927500,
	11327500,
	12827500,
	14427500
};

} // namespace

int calc_boss_value(CHAR_DATA *mob, int num)
{
	if (mob->get_role(MOB_ROLE_BOSS))
	{
		num += num * 25 / 100;
	}
	return num;
}

void set_test_data(CHAR_DATA *mob)
{
	if (!mob)
	{
		log("SYSERROR: null mob (%s %s %d)", __FILE__, __func__, __LINE__);
		return;
	}
	if (GET_EXP(mob) == 0)
	{
		return;
	}

	if (GET_EXP(mob) > test_levels[49])
	{
		// log("test1: %s - %d -> %d", mob->get_name(), mob->get_level(), 50);
		mob->set_level(50);
	}
	else
	{
		if (mob->get_level() == 0)
		{
			for (int i = 0; i < 50; ++i)
			{
				if (test_levels[i] >= GET_EXP(mob))
				{
					// log("test2: %s - %d -> %d", mob->get_name(), mob->get_level(), i + 1);
					mob->set_level(i + 1);
					break;
				}
			}
		}
	}

	if (mob->get_level() > 30)
	{
		// -10..-86
		int min_save = -(10 + 4 * (mob->get_level() - 31));
		min_save = calc_boss_value(mob, min_save);

		for (int i = 0; i < 4; ++i)
		{
			if (GET_SAVE(mob, i) > min_save)
			{
				//log("test3: %s - %d -> %d", mob->get_name(), GET_SAVE(mob, i), min_save);
				GET_SAVE(mob, i) = min_save;
			}
		}
		// 20..77
		int min_cast = 20 + 3 * (mob->get_level() - 31);
		min_cast = calc_boss_value(mob, min_cast);

		if (GET_CAST_SUCCESS(mob) < min_cast)
		{
			//log("test4: %s - %d -> %d", mob->get_name(), GET_CAST_SUCCESS(mob), min_cast);
			GET_CAST_SUCCESS(mob) = min_cast;
		}
	}

	// ���������� ���� ������������� ����
	GET_ABSORBE(mob) = calc_boss_value(mob, mob->get_level());
}

int csort(const void *a, const void *b)
{
	const struct social_keyword *a1, *b1;

	a1 = (const struct social_keyword *) a;
	b1 = (const struct social_keyword *) b;

	return (str_cmp(a1->keyword, b1->keyword));
}


/*************************************************************************
*  procedures for resetting, both play-time and boot-time                *
*************************************************************************/



int vnum_mobile(char *searchname, CHAR_DATA * ch)
{
	int nr, found = 0;

	for (nr = 0; nr <= top_of_mobt; nr++)
	{
		if (isname(searchname, mob_proto[nr].get_pc_name()))
		{
			sprintf(buf, "%3d. [%5d] %s\r\n", ++found, mob_index[nr].vnum, mob_proto[nr].get_npc_name().c_str());
			send_to_char(buf, ch);
		}
	}
	return (found);
}

int vnum_object(char *searchname, CHAR_DATA * ch)
{
	int found = 0;

	for (size_t nr = 0; nr < obj_proto.size(); nr++)
	{
		if (isname(searchname, obj_proto[nr]->get_aliases()))
		{
			++found;
			sprintf(buf, "%3d. [%5d] %s\r\n",
				found, obj_proto[nr]->get_vnum(),
				obj_proto[nr]->get_short_description().c_str());
			send_to_char(buf, ch);
		}
	}
	return (found);
}

int vnum_flag(char *searchname, CHAR_DATA * ch)
{
	int found = 0, plane = 0, counter = 0, plane_offset = 0;
	bool f = FALSE;
// type:
// 0 -- ����������� ���
// 1 -- �������
// 2 -- ����
// 4 -- �������
// ���� ��� �������� � �������: extra_bits[], apply_types[], weapon_affects[]
// ���� ��� ����� � �������  action_bits[], function_bits[],  affected_bits[], preference_bits[]
// ���� ��� ������ � ������� room_bits[]
	std::string out;

	for (counter = 0, plane = 0, plane_offset = 0; plane < NUM_PLANES; counter++)
	{
		if (*extra_bits[counter] == '\n')
		{
			plane++;
			plane_offset = 0;
			continue;
		};
		if (is_abbrev(searchname, extra_bits[counter]))
		{
			f = TRUE;
			break;
		}
		plane_offset++;
	}

	if (f)
	{
		for (const auto i : obj_proto)
		{
			if (i->get_extra_flag(plane, 1 << plane_offset))
			{
				snprintf(buf, MAX_STRING_LENGTH, "%3d. [%5d] %s :   %s\r\n",
					++found, i->get_vnum(), i->get_short_description().c_str(), extra_bits[counter]);
				out += buf;
			}
		}
	}
	f = FALSE;
// ---------------------
	for (counter = 0; *apply_types[counter] != '\n'; counter++)
	{
		if (is_abbrev(searchname, apply_types[counter]))
		{
			f = TRUE;
			break;
		}
	}
	if (f)
	{
		for (const auto i : obj_proto)
		{
			for (plane = 0; plane < MAX_OBJ_AFFECT; plane++)
			{
				if (i->get_affected(plane).location == static_cast<EApplyLocation>(counter))
				{
					snprintf(buf, MAX_STRING_LENGTH, "%3d. [%5d] %s : %s,  ��������: %d\r\n",
						++found, i->get_vnum(),
						i->get_short_description().c_str(),
						apply_types[counter], i->get_affected(plane).modifier);
					out += buf;
					continue;
				}
			}
		}
	}
	f = FALSE;
// ---------------------
	for (counter = 0, plane = 0, plane_offset = 0; plane < NUM_PLANES; counter++)
	{
		if (*weapon_affects[counter] == '\n')
		{
			plane++;
			plane_offset = 0;
			continue;
		};
		if (is_abbrev(searchname, weapon_affects[counter]))
		{
			f = TRUE;
			break;
		}
		plane_offset++;
	}
	if (f)
	{
		for (const auto i : obj_proto)
		{
			if (i->get_affect_flags().get_flag(plane, 1 << plane_offset))
			{
				snprintf(buf, MAX_STRING_LENGTH, "%3d. [%5d] %s :   %s\r\n",
					++found, i->get_vnum(),
					i->get_short_description().c_str(),
					weapon_affects[counter]);
				out += buf;
			}
		}
	}

	if (!out.empty())
	{
		page_string(ch->desc, out);
	}
	return found;
}

int vnum_room(char *searchname, CHAR_DATA * ch)
{
	int nr, found = 0;

	for (nr = 0; nr <= top_of_world; nr++)
	{
		if (isname(searchname, world[nr]->name))
		{
			sprintf(buf, "%3d. [%5d] %s\r\n", ++found, world[nr]->number, world[nr]->name);
			send_to_char(buf, ch);
		}
	}
	return (found);
}

namespace {

int test_hp[] = {
	20,
	40,
	60,
	80,
	100,
	120,
	140,
	160,
	180,
	200,
	220,
	240,
	260,
	280,
	300,
	340,
	380,
	420,
	460,
	520,
	580,
	640,
	700,
	760,
	840,
	1000,
	1160,
	1320,
	1480,
	1640,
	1960,
	2280,
	2600,
	2920,
	3240,
	3880,
	4520,
	5160,
	5800,
	6440,
	7720,
	9000,
	10280,
	11560,
	12840,
	15400,
	17960,
	20520,
	23080,
	25640
};

} // namespace

int get_test_hp(int lvl)
{
	if (lvl > 0 && lvl <= 50)
	{
		return test_hp[lvl - 1];
	}
	return 1;
}

// create a new mobile from a prototype
CHAR_DATA *read_mobile(mob_vnum nr, int type)
{				// and mob_rnum
	int is_corpse = 0;
	mob_rnum i;

	if (nr < 0)
	{
		is_corpse = 1;
		nr = -nr;
	}

	if (type == VIRTUAL)
	{
		if ((i = real_mobile(nr)) < 0)
		{
			log("WARNING: Mobile vnum %d does not exist in database.", nr);
			return (NULL);
		}
	}
	else
		i = nr;

	CHAR_DATA *mob = new CHAR_DATA;
	*mob = mob_proto[i]; //��� ��� ������� ��� ����������� ���� ���� �� �������� ��� ������...
	mob->set_normal_morph();
	mob->proto_script.reset(new OBJ_DATA::triggers_list_t());
	mob->set_next(character_list);
	character_list = mob;
//	CharacterAlias::add(mob);

	if (!mob->points.max_hit)
	{
		mob->points.max_hit = std::max(1,
			dice(GET_MEM_TOTAL(mob), GET_MEM_COMPLETED(mob)) + mob->points.hit);
	}
	else
	{
		mob->points.max_hit = std::max(1,
			number(mob->points.hit, GET_MEM_TOTAL(mob)));
	}

	int test_hp = get_test_hp(GET_LEVEL(mob));
	if (GET_EXP(mob) > 0 && mob->points.max_hit < test_hp)
	{
//		log("hp: (%s) %d -> %d", GET_NAME(mob), mob->points.max_hit, test_hp);
		mob->points.max_hit = test_hp;
	}

	mob->points.hit = mob->points.max_hit;
	GET_MEM_TOTAL(mob) = GET_MEM_COMPLETED(mob) = 0;
	GET_HORSESTATE(mob) = 800;
	GET_LASTROOM(mob) = NOWHERE;
	if (mob->mob_specials.speed <= -1)
		GET_ACTIVITY(mob) = number(0, PULSE_MOBILE - 1);
	else
		GET_ACTIVITY(mob) = number(0, mob->mob_specials.speed);
	EXTRACT_TIMER(mob) = 0;
	mob->points.move = mob->points.max_move;
	mob->add_gold(dice(GET_GOLD_NoDs(mob), GET_GOLD_SiDs(mob)));

	mob->player_data.time.birth = time(0);
	mob->player_data.time.played = 0;
	mob->player_data.time.logon = time(0);

	mob->id = max_id.allocate();

	if (!is_corpse)
	{
		mob_index[i].number++;
		assign_triggers(mob, MOB_TRIGGER);
	}

	i = mob_index[i].zone;
	if (i != -1 && zone_table[i].under_construction)
	{
		// mobile ����������� �������� ����
		MOB_FLAGS(mob).set(MOB_NOSUMMON);
	}

//Polud - �������� ���� ���������
	guardian_type::iterator it = guardian_list.find(GET_MOB_VNUM(mob));
	if (it != guardian_list.end())
	{
		MOB_FLAGS(mob).set(MOB_GUARDIAN);
	}

	return (mob);
}

/**
// ������� ��� �� �����, ��������� � ��������� ���� � ���������� ������ �������� ������
// �� ������ ������ ����������� ��������� �� ��������
 * \param type �� ������� VIRTUAL
 */
CObjectPrototype::shared_ptr get_object_prototype(obj_vnum nr, int type)
{
	unsigned i = nr;
	if (type == VIRTUAL)
	{
		const int rnum = real_object(nr);
		if (rnum < 0)
		{
			log("Object (V) %d does not exist in database.", nr);
			return nullptr;
		}
		else
		{
			i = rnum;
		}
	}

	if (i > obj_proto.size())
	{
		return 0;
	}
	return obj_proto[i];
}

// ��������� �� ���� ������� ���� � ������� ��� �����/��������, ���� ��� ����, ������ used �� true
void after_reset_zone(int nr_zone)
{
	// ��������� �� ������������, ��� ��� ������� � �����, �.�. � ���� ����� ���� � 200 � 300 �����
	DESCRIPTOR_DATA *d;
	for (d = descriptor_list; d; d = d->next)
	{
		// ��� ������ ���� � ����
		if (STATE(d) == CON_PLAYING)
		{
			if (world[d->character->in_room]->zone == nr_zone)
			{
				zone_table[nr_zone].used = true;
				return;
			}
		}
	}
}

#define ZO_DEAD  9999

// update zone ages, queue for reset if necessary, and dequeue when possible
void zone_update(void)
{
	int i, j, k = 0;
	struct reset_q_element *update_u, *temp;
	static int timer = 0;

	// jelson 10/22/92
	if (((++timer * PULSE_ZONE) / PASSES_PER_SEC) >= 60)  	// one minute has passed
	{
		/*
		 * NOT accurate unless PULSE_ZONE is a multiple of PASSES_PER_SEC or a
		 * factor of 60
		 */

		timer = 0;

		// since one minute has passed, increment zone ages
		for (i = 0; i <= top_of_zone_table; i++)
		{
			if (zone_table[i].age < zone_table[i].lifespan && zone_table[i].reset_mode &&
					(zone_table[i].reset_idle || zone_table[i].used))
				(zone_table[i].age)++;

			if (zone_table[i].age >= zone_table[i].lifespan &&
					zone_table[i].age < ZO_DEAD && zone_table[i].reset_mode &&
					(zone_table[i].reset_idle || zone_table[i].used))  	// enqueue zone
			{

				CREATE(update_u, 1);
				update_u->zone_to_reset = i;
				update_u->next = 0;

				if (!reset_q.head)
					reset_q.head = reset_q.tail = update_u;
				else
				{
					reset_q.tail->next = update_u;
					reset_q.tail = update_u;
				}

				zone_table[i].age = ZO_DEAD;
			}
		}
	}

	// end - one minute has passed
	// dequeue zones (if possible) and reset
	// this code is executed every 10 seconds (i.e. PULSE_ZONE)
	for (update_u = reset_q.head; update_u; update_u = update_u->next)
		if (zone_table[update_u->zone_to_reset].reset_mode == 2
				|| (is_empty(update_u->zone_to_reset) && zone_table[update_u->zone_to_reset].reset_mode != 3)
				|| can_be_reset(update_u->zone_to_reset))
		{
			reset_zone(update_u->zone_to_reset);
			char tmp[128];
			snprintf(tmp, sizeof(tmp), "Auto zone reset: %s (%d)",
				zone_table[update_u->zone_to_reset].name,
				zone_table[update_u->zone_to_reset].number);
			std::string out(tmp);
			if (zone_table[update_u->zone_to_reset].reset_mode == 3)
			{
				for (i = 0; i < zone_table[update_u->zone_to_reset].typeA_count; i++)
				{
					//���� real_zone �� vnum
					for (j = 0; j <= top_of_zone_table; j++)
					{
						if (zone_table[j].number ==
							zone_table[update_u->zone_to_reset].typeA_list[i])
						{
							reset_zone(j);
							snprintf(tmp, sizeof(tmp),
								" ]\r\n[ Also resetting: %s (%d)",
								zone_table[j].name, zone_table[j].number);
							out += tmp;
							break;
						}
					}
				}
			}
			mudlog(out.c_str(), LGH, LVL_GOD, SYSLOG, FALSE);
			// dequeue
			if (update_u == reset_q.head)
				reset_q.head = reset_q.head->next;
			else
			{
				for (temp = reset_q.head; temp->next != update_u; temp = temp->next);

				if (!update_u->next)
					reset_q.tail = temp;

				temp->next = update_u->next;
			}

			free(update_u);
			k++;
			if (k >= ZONES_RESET)
				break;
		}
}

bool can_be_reset(zone_rnum zone)
{
	int i = 0, j = 0;
	if (zone_table[zone].reset_mode != 3)
		return FALSE;
// ��������� ����
	if (!is_empty(zone))
		return FALSE;
// ��������� ������ B
	for (i = 0; i < zone_table[zone].typeB_count; i++)
	{
		//���� real_zone �� vnum
		for (j = 0; j <= top_of_zone_table; j++)
		{
			if (zone_table[j].number == zone_table[zone].typeB_list[i])
			{
				if (!zone_table[zone].typeB_flag[i] || !is_empty(j))
					return FALSE;
				break;
			}
		}
	}
// ��������� ������ A
	for (i = 0; i < zone_table[zone].typeA_count; i++)
	{
		//���� real_zone �� vnum
		for (j = 0; j <= top_of_zone_table; j++)
		{
			if (zone_table[j].number == zone_table[zone].typeA_list[i])
			{
				if (!is_empty(j))
					return FALSE;
				break;
			}
		}
	}
	return TRUE;
}

void paste_mob(CHAR_DATA *ch, room_rnum room)
{
	if (!IS_NPC(ch) || ch->get_fighting() || GET_POS(ch) < POS_STUNNED)
		return;
	if (IS_CHARMICE(ch)
		|| AFF_FLAGGED(ch, EAffectFlag::AFF_HORSE)
		|| AFF_FLAGGED(ch, EAffectFlag::AFF_HOLD)
		|| (EXTRACT_TIMER(ch) > 0))
	{
		return;
	}
//	if (MOB_FLAGGED(ch, MOB_CORPSE))
//		return;
	if (room == NOWHERE)
		return;

	bool time_ok = FALSE;
	bool month_ok = FALSE;
	bool need_move = FALSE;
	bool no_month = TRUE;
	bool no_time = TRUE;

	if (MOB_FLAGGED(ch, MOB_LIKE_DAY))
	{
		if (weather_info.sunlight == SUN_RISE || weather_info.sunlight == SUN_LIGHT)
			time_ok = TRUE;
		need_move = TRUE;
		no_time = FALSE;
	}
	if (MOB_FLAGGED(ch, MOB_LIKE_NIGHT))
	{
		if (weather_info.sunlight == SUN_SET || weather_info.sunlight == SUN_DARK)
			time_ok = TRUE;
		need_move = TRUE;
		no_time = FALSE;
	}
	if (MOB_FLAGGED(ch, MOB_LIKE_FULLMOON))
	{
		if ((weather_info.sunlight == SUN_SET ||
				weather_info.sunlight == SUN_DARK) &&
				(weather_info.moon_day >= 12 && weather_info.moon_day <= 15))
			time_ok = TRUE;
		need_move = TRUE;
		no_time = FALSE;
	}
	if (MOB_FLAGGED(ch, MOB_LIKE_WINTER))
	{
		if (weather_info.season == SEASON_WINTER)
			month_ok = TRUE;
		need_move = TRUE;
		no_month = FALSE;
	}
	if (MOB_FLAGGED(ch, MOB_LIKE_SPRING))
	{
		if (weather_info.season == SEASON_SPRING)
			month_ok = TRUE;
		need_move = TRUE;
		no_month = FALSE;
	}
	if (MOB_FLAGGED(ch, MOB_LIKE_SUMMER))
	{
		if (weather_info.season == SEASON_SUMMER)
			month_ok = TRUE;
		need_move = TRUE;
		no_month = FALSE;
	}
	if (MOB_FLAGGED(ch, MOB_LIKE_AUTUMN))
	{
		if (weather_info.season == SEASON_AUTUMN)
			month_ok = TRUE;
		need_move = TRUE;
		no_month = FALSE;
	}
	if (need_move)
	{
		month_ok |= no_month;
		time_ok |= no_time;
		if (month_ok && time_ok)
		{
			if (world[room]->number != zone_table[world[room]->zone].top)
				return;
			if (GET_LASTROOM(ch) == NOWHERE)
			{
				extract_mob(ch);
				return;
			}
			char_from_room(ch);
			char_to_room(ch, real_room(GET_LASTROOM(ch)));
		}
		else
		{
			if (world[room]->number == zone_table[world[room]->zone].top)
				return;
			GET_LASTROOM(ch) = GET_ROOM_VNUM(room);
			char_from_room(ch);
			room = real_room(zone_table[world[room]->zone].top);
			if (room == NOWHERE)
				room = real_room(GET_LASTROOM(ch));
			char_to_room(ch, room);
		}
	}
}

void paste_obj(OBJ_DATA *obj, room_rnum room)
{
	if (obj->get_carried_by()
		|| obj->get_worn_by()
		|| room == NOWHERE)
	{
		return;
	}

	bool time_ok = FALSE;
	bool month_ok = FALSE;
	bool need_move = FALSE;
	bool no_time = TRUE;
	bool no_month = TRUE;

	if (OBJ_FLAGGED(obj, EExtraFlag::ITEM_DAY))
	{
		if (weather_info.sunlight == SUN_RISE
			|| weather_info.sunlight == SUN_LIGHT)
		{
			time_ok = TRUE;
		}
		need_move = TRUE;
		no_time = FALSE;
	}
	if (OBJ_FLAGGED(obj, EExtraFlag::ITEM_NIGHT))
	{
		if (weather_info.sunlight == SUN_SET
			|| weather_info.sunlight == SUN_DARK)
		{
			time_ok = TRUE;
		}
		need_move = TRUE;
		no_time = FALSE;
	}
	if (OBJ_FLAGGED(obj, EExtraFlag::ITEM_FULLMOON))
	{
		if ((weather_info.sunlight == SUN_SET
				|| weather_info.sunlight == SUN_DARK)
			&& weather_info.moon_day >= 12
			&& weather_info.moon_day <= 15)
		{
			time_ok = TRUE;
		}
		need_move = TRUE;
		no_time = FALSE;
	}
	if (OBJ_FLAGGED(obj, EExtraFlag::ITEM_WINTER))
	{
		if (weather_info.season == SEASON_WINTER)
		{
			month_ok = TRUE;
		}
		need_move = TRUE;
		no_month = FALSE;
	}
	if (OBJ_FLAGGED(obj, EExtraFlag::ITEM_SPRING))
	{
		if (weather_info.season == SEASON_SPRING)
		{
			month_ok = TRUE;
		}
		need_move = TRUE;
		no_month = FALSE;
	}
	if (OBJ_FLAGGED(obj, EExtraFlag::ITEM_SUMMER))
	{
		if (weather_info.season == SEASON_SUMMER)
		{
			month_ok = TRUE;
		}
		need_move = TRUE;
		no_month = FALSE;
	}
	if (OBJ_FLAGGED(obj, EExtraFlag::ITEM_AUTUMN))
	{
		if (weather_info.season == SEASON_AUTUMN)
		{
			month_ok = TRUE;
		}
		need_move = TRUE;
		no_month = FALSE;
	}
	if (need_move)
	{
		month_ok |= no_month;
		time_ok |= no_time;
		if (month_ok && time_ok)
		{
			if (world[room]->number != zone_table[world[room]->zone].top)
			{
				return;
			}
			if (OBJ_GET_LASTROOM(obj) == NOWHERE)
			{
				extract_obj(obj);
				return;
			}
			obj_from_room(obj);
			obj_to_room(obj, real_room(OBJ_GET_LASTROOM(obj)));
		}
		else
		{
			if (world[room]->number == zone_table[world[room]->zone].top)
			{
				return;
			}
			obj->set_room_was_in(GET_ROOM_VNUM(room));
			obj_from_room(obj);
			room = real_room(zone_table[world[room]->zone].top);
			if (room == NOWHERE)
			{
				room = real_room(OBJ_GET_LASTROOM(obj));
			}
			obj_to_room(obj, room);
		}
	}
}

void paste_mobiles()
{
	CHAR_DATA *ch_next;
	for (CHAR_DATA *ch = character_list; ch; ch = ch_next)
	{
		ch_next = ch->get_next();
		paste_mob(ch, ch->in_room);
	}

	world_objects.foreach_on_copy([](const OBJ_DATA::shared_ptr& object)
	{
		paste_obj(object.get(), object->get_in_room());
	});
}

void paste_on_reset(ROOM_DATA *to_room)
{
	CHAR_DATA *ch_next;
	for (CHAR_DATA *ch = to_room->people; ch != NULL; ch = ch_next)
	{
		ch_next = ch->next_in_room;
		paste_mob(ch, ch->in_room);
	}

	OBJ_DATA *obj_next;
	for (OBJ_DATA *obj = to_room->contents; obj; obj = obj_next)
	{
		obj_next = obj->get_next_content();
		paste_obj(obj, obj->get_in_room());
	}
}

void log_zone_error(zone_rnum zone, int cmd_no, const char *message)
{
	char buf[256];

	sprintf(buf, "SYSERR: zone file %d.zon: %s", zone_table[zone].number, message);
	mudlog(buf, NRM, LVL_GOD, SYSLOG, TRUE);

	sprintf(buf, "SYSERR: ...offending cmd: '%c' cmd in zone #%d, line %d",
			ZCMD.command, zone_table[zone].number, ZCMD.line);
	mudlog(buf, NRM, LVL_GOD, SYSLOG, TRUE);
}

void process_load_celebrate(Celebrates::CelebrateDataPtr celebrate, int vnum)
{
	Celebrates::CelebrateRoomsList::iterator room;
	Celebrates::LoadList::iterator load, load_in;

	log("Processing celebrate %s load section for zone %d", celebrate->name.c_str(), vnum);

	if (celebrate->rooms.find(vnum) != celebrate->rooms.end())
	{
		for (room = celebrate->rooms[vnum].begin();room != celebrate->rooms[vnum].end(); ++room)
		{
			room_rnum rn = real_room((*room)->vnum);
			if ( rn != NOWHERE)
			{
				if (!(world[rn]->script))
					CREATE(world[rn]->script, 1);

				for (Celebrates::TrigList::iterator it = (*room)->triggers.begin();
						it != (*room)->triggers.end(); ++it)
				{
					add_trigger(world[rn]->script, read_trigger(real_trigger(*it)), -1);
				}
			}

			for (load = (*room)->mobs.begin(); load != (*room)->mobs.end(); ++load)
			{
				CHAR_DATA* mob;
				int i = real_mobile((*load)->vnum);
				if (i > 0
					&& mob_index[i].number < (*load)->max)
				{
					mob = read_mobile(i, REAL);
					if (mob)
					{
						if (!SCRIPT(mob))
						{
							CREATE(SCRIPT(mob), 1);
						}
						for (Celebrates::TrigList::iterator it = (*load)->triggers.begin();
							it != (*load)->triggers.end(); ++it)
						{
							add_trigger(SCRIPT(mob), read_trigger(real_trigger(*it)), -1);
						}
						load_mtrigger(mob);
						char_to_room(mob, real_room((*room)->vnum));
						Celebrates::add_mob_to_load_list(mob->id, mob);
						for (load_in = (*load)->objects.begin(); load_in != (*load)->objects.end(); ++load_in)
						{
							obj_rnum rnum = real_object((*load_in)->vnum);

							if (obj_proto.actual_count(rnum) < obj_proto[rnum]->get_max_in_world())
							{
								const auto obj = world_objects.create_from_prototype_by_vnum((*load_in)->vnum);
								if (obj)
			 					{
									obj_to_char(obj.get(), mob);
									obj->set_zone(world[IN_ROOM(mob)]->zone);

									if (!obj->get_script())
									{
										obj->set_script(new SCRIPT_DATA());
									}
									for (Celebrates::TrigList::iterator it = (*load_in)->triggers.begin();
											it != (*load_in)->triggers.end(); ++it)
									{
										add_trigger(obj->get_script().get(), read_trigger(real_trigger(*it)), -1);
									}

									load_otrigger(obj.get());
									Celebrates::add_obj_to_load_list(obj->get_uid(), obj.get());
								}
								else
								{
									log("{Error] Processing celebrate %s while loading obj %d", celebrate->name.c_str(), (*load_in)->vnum);
								}
							}
						}
					}
					else
					{
						log("{Error] Processing celebrate %s while loading mob %d", celebrate->name.c_str(), (*load)->vnum);
					}
				}
			}
			for (load = (*room)->objects.begin(); load != (*room)->objects.end(); ++load)
			{
				OBJ_DATA *obj_room;
				obj_rnum rnum = real_object((*load)->vnum);
				if (rnum == -1)
				{
					log("{Error] Processing celebrate %s while loading obj %d", celebrate->name.c_str(), (*load)->vnum);
					return;
				}
				int obj_in_room = 0;

				for (obj_room = world[rn]->contents; obj_room; obj_room = obj_room->get_next_content())
				{
					if (rnum == GET_OBJ_RNUM(obj_room))
					{
						obj_in_room++;
					}
				}

				if ((obj_proto.actual_count(rnum) < obj_proto[rnum]->get_max_in_world())
					&& (obj_in_room < (*load)->max))
				{
					const auto obj = world_objects.create_from_prototype_by_vnum((*load)->vnum);
					if (obj)
					{
						if (!obj->get_script())
						{
							obj->set_script(new SCRIPT_DATA());
						}
						for (Celebrates::TrigList::iterator it = (*load)->triggers.begin();
							it != (*load)->triggers.end(); ++it)
						{
							add_trigger(obj->get_script().get(), read_trigger(real_trigger(*it)), -1);
						}
						load_otrigger(obj.get());
						Celebrates::add_obj_to_load_list(obj->get_uid(), obj.get());

						obj_to_room(obj.get(), real_room((*room)->vnum));

						for (load_in = (*load)->objects.begin(); load_in != (*load)->objects.end(); ++load_in)
						{
							obj_rnum rnum = real_object((*load_in)->vnum);

							if (obj_proto.actual_count(rnum) < obj_proto[rnum]->get_max_in_world())
							{
								const auto obj_in = world_objects.create_from_prototype_by_vnum((*load_in)->vnum);
								if (obj_in
									&& GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_CONTAINER)
			 					{
									obj_to_obj(obj_in.get(), obj.get());
									obj_in->set_zone(GET_OBJ_ZONE(obj));

									if (!obj_in->get_script())
									{
										obj_in->set_script(new SCRIPT_DATA());
									}
									for (Celebrates::TrigList::iterator it = (*load_in)->triggers.begin();
											it != (*load_in)->triggers.end(); ++it)
									{
										add_trigger(obj_in->get_script().get(), read_trigger(real_trigger(*it)), -1);
									}

									load_otrigger(obj_in.get());
									Celebrates::add_obj_to_load_list(obj->get_uid(), obj.get());
								}
								else
								{
									log("{Error] Processing celebrate %s while loading obj %d", celebrate->name.c_str(), (*load_in)->vnum);
								}
							}
						}
					}
					else
					{
						log("{Error] Processing celebrate %s while loading mob %d", celebrate->name.c_str(), (*load)->vnum);
					}
				}
			}
		}
	}
}

void process_attach_celebrate(Celebrates::CelebrateDataPtr celebrate, int zone_vnum)
{
	log("Processing celebrate %s attach section for zone %d", celebrate->name.c_str(), zone_vnum);

	if (celebrate->mobsToAttach.find(zone_vnum) != celebrate->mobsToAttach.end())
	{
		//��������� ������������ ��������� �������� �������� ���� ����� ������ ����� ��������
		//����� ����� ������ ����� � ����, �� ����� ���� �� 1 ��� ��� ��������
		Celebrates::AttachList list = celebrate->mobsToAttach[zone_vnum];
		for (CHAR_DATA *ch = character_list; ch; ch=ch->get_next())
		{
			if (ch->nr > 0 && list.find(mob_index[ch->nr].vnum) != list.end())
			{
				if (!SCRIPT(ch))
					CREATE(SCRIPT(ch), 1);
				for (Celebrates::TrigList::iterator it = list[mob_index[ch->nr].vnum].begin();
						it != list[mob_index[ch->nr].vnum].end(); ++it)
					add_trigger(SCRIPT(ch), read_trigger(real_trigger(*it)), -1);
				Celebrates::add_mob_to_attach_list(ch->id, ch);
			}
		}
	}

	if (celebrate->objsToAttach.find(zone_vnum) != celebrate->objsToAttach.end())
	{
		Celebrates::AttachList list = celebrate->objsToAttach[zone_vnum];

		world_objects.foreach([&](const OBJ_DATA::shared_ptr& o)
		{
			if (o->get_rnum() > 0 && list.find(o->get_rnum()) != list.end())
			{
				if (!o->get_script())
				{
					o->set_script(new SCRIPT_DATA());
				}

				for (Celebrates::TrigList::iterator it = list[o->get_rnum()].begin(); it != list[o->get_rnum()].end(); ++it)
				{
					add_trigger(o->get_script().get(), read_trigger(real_trigger(*it)), -1);
				}

				Celebrates::add_obj_to_attach_list(o->get_uid(), o.get());
			}
		});
	}
}

void process_celebrates(int vnum)
{
	Celebrates::CelebrateDataPtr mono = Celebrates::get_mono_celebrate();
	Celebrates::CelebrateDataPtr poly = Celebrates::get_poly_celebrate();
	Celebrates::CelebrateDataPtr real = Celebrates::get_real_celebrate();

	if (mono)
	{
		process_load_celebrate(mono, vnum);
		process_attach_celebrate(mono, vnum);
	}

	if (poly)
	{
		process_load_celebrate(poly, vnum);
		process_attach_celebrate(poly, vnum);
	}
	if (real)
	{
		process_load_celebrate(real, vnum);
		process_attach_celebrate(real, vnum);
	}
}

#define ZONE_ERROR(message) \
        { log_zone_error(zone, cmd_no, message); }

// �������� �������, ������ ���� ���������� �������
#define		CHECK_SUCCESS		1
// ������� �� ������ �������� ����
#define		FLAG_PERSIST		2

// execute the reset command table of a given zone
void reset_zone(zone_rnum zone)
{
	int cmd_no;
	int cmd_tmp, obj_in_room_max, obj_in_room = 0;
	CHAR_DATA *mob = NULL, *leader = NULL, *ch;
	OBJ_DATA *obj_to, *obj_room;
	int rnum_start, rnum_stop;
	CHAR_DATA *tmob = NULL;	// for trigger assignment
	OBJ_DATA *tobj = NULL;	// for trigger assignment

	int last_state, curr_state;	// ������ ���������� ��������� � ������� �������

	log("[Reset] Start zone %s", zone_table[zone].name);
	repop_decay(zone);	// ���������� �������� ITEM_REPOP_DECAY

	//----------------------------------------------------------------------------
	last_state = 1;		// ��� ������ ������� �������, ��� ��� ��

	for (cmd_no = 0; ZCMD.command != 'S'; cmd_no++)
	{
		if (ZCMD.command == '*')
		{
			// ����������� - �� �� ��� �� ������
			continue;
		}

		curr_state = 0;	// �� ��������� - ��������
		if (!(ZCMD.if_flag & CHECK_SUCCESS) || last_state)
		{
			// ��������� �������, ���� ���������� ������� ��� �� ����� ���������
			switch (ZCMD.command)
			{
			case 'M':
				// read a mobile
				// 'M' <flag> <mob_vnum> <max_in_world> <room_vnum> <max_in_room|-1>
				mob = NULL;	//��������� ��������
				if (mob_index[ZCMD.arg1].number < ZCMD.arg2 &&
						(ZCMD.arg4 < 0 || mobs_in_room(ZCMD.arg1, ZCMD.arg3) < ZCMD.arg4))
				{
					mob = read_mobile(ZCMD.arg1, REAL);
					char_to_room(mob, ZCMD.arg3);
					load_mtrigger(mob);
					tmob = mob;
					curr_state = 1;
				}
				tobj = NULL;
				break;

			case 'F':
				// Follow mobiles
				// 'F' <flag> <room_vnum> <leader_vnum> <mob_vnum>
				leader = NULL;
				if (ZCMD.arg1 >= FIRST_ROOM && ZCMD.arg1 <= top_of_world)
				{
					for (ch = world[ZCMD.arg1]->people; ch && !leader; ch = ch->next_in_room)
					{
						if (IS_NPC(ch) && GET_MOB_RNUM(ch) == ZCMD.arg2)
						{
							leader = ch;
						}
					}

					for (ch = world[ZCMD.arg1]->people; ch && leader; ch = ch->next_in_room)
					{
						if (IS_NPC(ch)
							&& GET_MOB_RNUM(ch) == ZCMD.arg3
							&& leader != ch
							&& !ch->makes_loop(leader))
						{
							if (ch->has_master())
							{
								stop_follower(ch, SF_EMPTY);
							}

							leader->add_follower(ch);

							curr_state = 1;
						}
					}
				}
				break;

			case 'Q':
				// delete all mobiles
				// 'Q' <flag> <mob_vnum>
				for (ch = character_list; ch; ch = leader)
				{
					leader = ch->get_next();
					// �������. �������� ���� �� ������ ������������.
					if (IS_NPC(ch) && GET_MOB_RNUM(ch) == ZCMD.arg1 && !MOB_FLAGGED(ch, MOB_RESURRECTED))
					{
						// �������. ���� ������ ��������� �����.
						// ���� ����� ����, � �� ���� �� ��� ������ ����. -- Krodo
						extract_char(ch, FALSE, 1);
						//extract_mob(ch);
						curr_state = 1;
					}
				}
				tobj = NULL;
				tmob = NULL;
				break;

			case 'O':
				// read an object
				// 'O' <flag> <obj_vnum> <max_in_world> <room_vnum|-1> <load%|-1>
				// ��������  - ������� ����� ����� �� �������� ���� �� ��� ������
				for (cmd_tmp = 0, obj_in_room_max = 0; ZCMD_CMD(cmd_tmp).command != 'S'; cmd_tmp++)
					if ((ZCMD_CMD(cmd_tmp).command == 'O')
							&& (ZCMD.arg1 == ZCMD_CMD(cmd_tmp).arg1)
							&& (ZCMD.arg3 == ZCMD_CMD(cmd_tmp).arg3))
						obj_in_room_max++;
				// ������ ������� ������ �� �� ������� ������
				if (ZCMD.arg3 >= 0)
				{
					for (obj_room = world[ZCMD.arg3]->contents, obj_in_room = 0; obj_room; obj_room = obj_room->get_next_content())
					{
						if (ZCMD.arg1 == GET_OBJ_RNUM(obj_room))
						{
							obj_in_room++;
						}
					}
				}
				// ������ ������ ������ ���� ����
				if ((obj_proto.actual_count(ZCMD.arg1) < GET_OBJ_MIW(obj_proto[ZCMD.arg1])
						|| GET_OBJ_MIW(obj_proto[ZCMD.arg1]) == OBJ_DATA::UNLIMITED_GLOBAL_MAXIMUM
						|| check_unlimited_timer(obj_proto[ZCMD.arg1].get()))
					&& (ZCMD.arg4 <= 0
						|| number(1, 100) <= ZCMD.arg4)
					&& (obj_in_room < obj_in_room_max))
				{
					const auto obj = world_objects.create_from_prototype_by_rnum(ZCMD.arg1);
					if (ZCMD.arg3 >= 0)
					{
						obj->set_zone(world[ZCMD.arg3]->zone);
						if (!obj_to_room(obj.get(), ZCMD.arg3))
						{
							extract_obj(obj.get());
							break;
						}
						load_otrigger(obj.get());
					}
					else
					{
						obj->set_in_room(NOWHERE);
					}
					tobj = obj.get();
					curr_state = 1;

					if (!obj->get_extra_flag(EExtraFlag::ITEM_NODECAY))
					{
						sprintf(buf, "&Y��������&G �� ����� �������� ������ ��� ����� NODECAY : %s (VNUM=%d)",
							GET_OBJ_PNAME(obj, 0).c_str(), obj->get_vnum());
						mudlog(buf, BRF, LVL_BUILDER, ERRLOG, TRUE);
					}
				}
				tmob = NULL;
				break;

			case 'P':
				// object to object
				// 'P' <flag> <obj_vnum> <max_in_world> <target_vnum> <load%|-1>
				if ((obj_proto.actual_count(ZCMD.arg1) < GET_OBJ_MIW(obj_proto[ZCMD.arg1])
						|| GET_OBJ_MIW(obj_proto[ZCMD.arg1]) == OBJ_DATA::UNLIMITED_GLOBAL_MAXIMUM
						|| check_unlimited_timer(obj_proto[ZCMD.arg1].get()))
					&& (ZCMD.arg4 <= 0
						|| number(1, 100) <= ZCMD.arg4))
				{
					if (!(obj_to = get_obj_num(ZCMD.arg3)))
					{
						ZONE_ERROR("target obj not found, command omited");
						break;
					}
					if (GET_OBJ_TYPE(obj_to) != OBJ_DATA::ITEM_CONTAINER)
					{
						ZONE_ERROR("attempt put obj to non container, omited");
						ZCMD.command = '*';
						break;
					}
					const auto obj = world_objects.create_from_prototype_by_rnum(ZCMD.arg1);
					if (obj_to->get_in_room() != NOWHERE)
					{
						obj->set_zone(world[obj_to->get_in_room()]->zone);
					}
					else if (obj_to->get_worn_by())
					{
						obj->set_zone(world[IN_ROOM(obj_to->get_worn_by())]->zone);
					}
					else if (obj_to->get_carried_by())
					{
						obj->set_zone(world[IN_ROOM(obj_to->get_carried_by())]->zone);
					}
					obj_to_obj(obj.get(), obj_to);
					load_otrigger(obj.get());
					tobj = obj.get();
					curr_state = 1;
				}
				tmob = NULL;
				break;

			case 'G':
				// obj_to_char
				// 'G' <flag> <obj_vnum> <max_in_world> <-> <load%|-1>
				if (!mob)
					//�������� ��������
				{
					// ZONE_ERROR("attempt to give obj to non-existant mob, command disabled");
					// ZCMD.command = '*';
					break;
				}
				if ((obj_proto.actual_count(ZCMD.arg1) < GET_OBJ_MIW(obj_proto[ZCMD.arg1])
						|| GET_OBJ_MIW(obj_proto[ZCMD.arg1]) == OBJ_DATA::UNLIMITED_GLOBAL_MAXIMUM
						|| check_unlimited_timer(obj_proto[ZCMD.arg1].get()))
					&& (ZCMD.arg4 <= 0
						|| number(1, 100) <= ZCMD.arg4))
				{
					const auto obj = world_objects.create_from_prototype_by_rnum(ZCMD.arg1);
					obj_to_char(obj.get(), mob);
					obj->set_zone(world[IN_ROOM(mob)]->zone);
					tobj = obj.get();
					load_otrigger(obj.get());
					curr_state = 1;
				}
				tmob = NULL;
				break;

			case 'E':
				// object to equipment list
				// 'E' <flag> <obj_vnum> <max_in_world> <wear_pos> <load%|-1>
				//�������� ��������
				if (!mob)
				{
					//ZONE_ERROR("trying to equip non-existant mob, command disabled");
					// ZCMD.command = '*';
					break;
				}
				if ((obj_proto.actual_count(ZCMD.arg1) < obj_proto[ZCMD.arg1]->get_max_in_world()
						|| GET_OBJ_MIW(obj_proto[ZCMD.arg1]) == OBJ_DATA::UNLIMITED_GLOBAL_MAXIMUM
						|| check_unlimited_timer(obj_proto[ZCMD.arg1].get()))
					&& (ZCMD.arg4 <= 0
						|| number(1, 100) <= ZCMD.arg4))
				{
					if (ZCMD.arg3 < 0
						|| ZCMD.arg3 >= NUM_WEARS)
					{
						ZONE_ERROR("invalid equipment pos number");
					}
					else
					{
						const auto obj = world_objects.create_from_prototype_by_rnum(ZCMD.arg1);
						obj->set_zone(world[IN_ROOM(mob)]->zone);
						obj->set_in_room(IN_ROOM(mob));
						load_otrigger(obj.get());
						if (wear_otrigger(obj.get(), mob, ZCMD.arg3))
						{
							obj->set_in_room(NOWHERE);
							equip_char(mob, obj.get(), ZCMD.arg3);
						}
						else
						{
							obj_to_char(obj.get(), mob);
						}
						if (!(obj->get_carried_by() == mob)
							&& !(obj->get_worn_by() == mob))
						{
							extract_obj(obj.get());
							tobj = NULL;
						}
						else
						{
							tobj = obj.get();
							curr_state = 1;
						}
					}
				}
				tmob = NULL;
				break;

			case 'R':
				// rem obj from room
				// 'R' <flag> <room_vnum> <obj_vnum>
				if (const auto obj = get_obj_in_list_num(ZCMD.arg2, world[ZCMD.arg1]->contents))
				{
					obj_from_room(obj);
					extract_obj(obj);
					curr_state = 1;
				}
				tmob = NULL;
				tobj = NULL;
				break;

			case 'D':
				// set state of door
				// 'D' <flag> <room_vnum> <door_pos> <door_state>
				if (ZCMD.arg2 < 0 || ZCMD.arg2 >= NUM_OF_DIRS ||
						(world[ZCMD.arg1]->dir_option[ZCMD.arg2] == NULL))
				{
					ZONE_ERROR("door does not exist, command disabled");
					ZCMD.command = '*';
				}
				else
				{
					REMOVE_BIT(world[ZCMD.arg1]->dir_option[ZCMD.arg2]->exit_info, EX_BROKEN);
					switch (ZCMD.arg3)
					{
					case 0:
						REMOVE_BIT(world[ZCMD.arg1]->dir_option[ZCMD.arg2]->
								   exit_info, EX_LOCKED);
						REMOVE_BIT(world[ZCMD.arg1]->dir_option[ZCMD.arg2]->
								   exit_info, EX_CLOSED);
						break;
					case 1:
						SET_BIT(world[ZCMD.arg1]->dir_option[ZCMD.arg2]->exit_info, EX_CLOSED);
						REMOVE_BIT(world[ZCMD.arg1]->dir_option[ZCMD.arg2]->
								   exit_info, EX_LOCKED);
						break;
					case 2:
						SET_BIT(world[ZCMD.arg1]->dir_option[ZCMD.arg2]->exit_info, EX_LOCKED);
						SET_BIT(world[ZCMD.arg1]->dir_option[ZCMD.arg2]->exit_info, EX_CLOSED);
						break;
					case 3:
						SET_BIT(world[ZCMD.arg1]->dir_option[ZCMD.arg2]->exit_info, EX_HIDDEN);
						break;
					case 4:
						REMOVE_BIT(world[ZCMD.arg1]->dir_option[ZCMD.arg2]->
								   exit_info, EX_HIDDEN);
						break;
					}
					curr_state = 1;
				}
				tmob = NULL;
				tobj = NULL;
				break;

			case 'T':
				// trigger command; details to be filled in later
				// 'T' <flag> <trigger_type> <trigger_vnum> <room_vnum, ��� WLD_TRIGGER>
				if (ZCMD.arg1 == MOB_TRIGGER && tmob)
				{
					if (!SCRIPT(tmob))
						CREATE(SCRIPT(tmob), 1);
					add_trigger(SCRIPT(tmob), read_trigger(real_trigger(ZCMD.arg2)), -1);
					curr_state = 1;
				}
				else if (ZCMD.arg1 == OBJ_TRIGGER && tobj)
				{
					if (!tobj->get_script())
					{
						tobj->set_script(new SCRIPT_DATA());
					}
					add_trigger(tobj->get_script().get(), read_trigger(real_trigger(ZCMD.arg2)), -1);
					curr_state = 1;
				}
				else if (ZCMD.arg1 == WLD_TRIGGER)
				{
					if (ZCMD.arg3 != NOWHERE)
					{
						if (!(world[ZCMD.arg3]->script))
						{
							CREATE(world[ZCMD.arg3]->script, 1);
						}
						add_trigger(world[ZCMD.arg3]->script,
									read_trigger(real_trigger(ZCMD.arg2)), -1);
						curr_state = 1;
					}
				}
				break;

			case 'V':
				// 'V' <flag> <trigger_type> <room_vnum> <context> <var_name> <var_value>
				if (ZCMD.arg1 == MOB_TRIGGER && tmob)
				{
					if (!SCRIPT(tmob))
					{
						ZONE_ERROR("Attempt to give variable to scriptless mobile");
					}
					else
					{
						add_var_cntx(&(SCRIPT(tmob)->global_vars), ZCMD.sarg1, ZCMD.sarg2, ZCMD.arg3);
						curr_state = 1;
					}
				}
				else if (ZCMD.arg1 == OBJ_TRIGGER && tobj)
				{
					if (!tobj->get_script())
					{
						ZONE_ERROR("Attempt to give variable to scriptless object");
					}
					else
					{
						add_var_cntx(&(tobj->get_script()->global_vars), ZCMD.sarg1, ZCMD.sarg2, ZCMD.arg3);
						curr_state = 1;
					}
				}
				else if (ZCMD.arg1 == WLD_TRIGGER)
				{
					if (ZCMD.arg2 < FIRST_ROOM || ZCMD.arg2 > top_of_world)
					{
						ZONE_ERROR("Invalid room number in variable assignment");
					}
					else
					{
						if (!(world[ZCMD.arg2]->script))
						{
							ZONE_ERROR("Attempt to give variable to scriptless object");
						}
						else
						{
							add_var_cntx(&(world[ZCMD.arg2]->script->global_vars), ZCMD.sarg1, ZCMD.sarg2, ZCMD.arg3);
							curr_state = 1;
						}
					}
				}
				break;

			default:
				ZONE_ERROR("unknown cmd in reset table; cmd disabled");
				ZCMD.command = '*';
				break;
			}
		}
		// if ( (ZCMD.if_flag&CHECK_SUCCESS) && !last_state )
		if (!(ZCMD.if_flag & FLAG_PERSIST))
		{
			// ������� �������� ����
			last_state = curr_state;
		}

	}
	if (zone_table[zone].used)
	{
		zone_table[zone].count_reset++;
	}
	zone_table[zone].age = 0;
	zone_table[zone].used = FALSE;
	process_celebrates(zone_table[zone].number);
	if (get_zone_rooms(zone, &rnum_start, &rnum_stop))
	{
		ROOM_DATA* room;
		ROOM_DATA* gate_room;
		// ��� ���������� ������ ������ ���� ������ ���� �� ���� ����
		// ����� �������� ������ ��� �� � ���������, ����� �� ������ �� ���� �����, ��� ���� �������� ������� ������ -- Krodo
		for (int rnum = rnum_start; rnum <= rnum_stop; rnum++)
		{
			room = world[rnum];
			reset_wtrigger(room);
			im_reset_room(room, zone_table[zone].level, zone_table[zone].type);
			gate_room = OneWayPortal::get_from_room(room);
			if (gate_room)   // ������ ����
			{
				gate_room->portal_time = 0;
				OneWayPortal::remove(room);
			}
			else if (room->portal_time > 0)   // ������ ������������ �����
			{
				world[room->portal_room]->portal_time = 0;
				room->portal_time = 0;
			}
			paste_on_reset(room);
		}
	}

	//process_celebrates(zone_table[zone].number);

	for (rnum_start = 0; rnum_start <= top_of_zone_table; rnum_start++)
	{
		// ���������, �� ���������� �� ������� ���� � ����-���� typeB_list
		for (curr_state = zone_table[rnum_start].typeB_count; curr_state > 0; curr_state--)
		{
			if (zone_table[rnum_start].typeB_list[curr_state - 1] == zone_table[zone].number)
			{
				zone_table[rnum_start].typeB_flag[curr_state - 1] = TRUE;
//				log("[Reset] Adding TRUE for zone %d in the array contained by zone %d",
//				    zone_table[zone].number, zone_table[rnum_start].number);
				break;
			}
		}
	}
	//���� ��� ������� ����, �� ��� �� ������ �������� typeB_flag
	for (rnum_start = zone_table[zone].typeB_count; rnum_start > 0; rnum_start--)
		zone_table[zone].typeB_flag[rnum_start - 1] = FALSE;
	log("[Reset] Stop zone %s", zone_table[zone].name);
	after_reset_zone(zone);
}

// ���� RNUM ������ � ��������� ������� ����
// ��� ���������� 0 - ������ � ���� ����
int get_zone_rooms(int zone_nr, int *start, int *stop)
{
	int first_room_vnum, rnum;
	first_room_vnum = zone_table[zone_nr].top;
	rnum = real_room(first_room_vnum);
	if (rnum == NOWHERE)
		return 0;
	*stop = rnum;
	rnum = NOWHERE;
	while (zone_nr)
	{
		first_room_vnum = zone_table[--zone_nr].top;
		rnum = real_room(first_room_vnum);
		if (rnum != NOWHERE)
		{
			++rnum;
			break;
		}
	}
	if (rnum == NOWHERE)
		rnum = 1;	// ����� ������ ���� ���������� � 1
	*start = rnum;
	return 1;
}


// for use in reset_zone; return TRUE if zone 'nr' is free of PC's
int is_empty(zone_rnum zone_nr)
{
	DESCRIPTOR_DATA *i;
	int rnum_start, rnum_stop;
	CHAR_DATA *c;
	//char *buf_tmp;
	for (i = descriptor_list; i; i = i->next)
	{
		if (STATE(i) != CON_PLAYING)
			continue;
		if (IN_ROOM(i->character) == NOWHERE)
			continue;
		if (GET_LEVEL(i->character) >= LVL_IMMORT)
			continue;
		if (world[i->character->in_room]->zone != zone_nr)
			continue;
		return (0);
	}

	// ����� link-dead ������� � ����� ������� zone_nr
	if (!get_zone_rooms(zone_nr, &rnum_start, &rnum_stop))
		return 1;	// � ���� ��� ������ :)

	for (; rnum_start <= rnum_stop; rnum_start++)
	{
// num_pc_in_room() ������������ ������, �.�. ������� ������ � ������.
		for (c = world[rnum_start]->people; c; c = c->next_in_room)
			if (!IS_NPC(c) && (GET_LEVEL(c) < LVL_IMMORT))
			{
				return 0;
			}
	}

// ������ ������� ���� ��������� � void ������� STRANGE_ROOM
	for (c = world[STRANGE_ROOM]->people; c; c = c->next_in_room)
	{
		int was = c->get_was_in_room();
		if (was == NOWHERE)
			continue;
		if (GET_LEVEL(c) >= LVL_IMMORT)
			continue;
		if (world[was]->zone != zone_nr)
			continue;
		return 0;
	}

//��������, ��� �� � ���� ����� ��� ����, ���� �� �������.
    for (auto it = RoomSpells::aff_room_list.begin(); it != RoomSpells::aff_room_list.end(); ++it)
	{
		if ((*it)->zone == zone_nr
			&& find_room_affect(*it, SPELL_RUNE_LABEL) != (*it)->affected.end())
		{
			// ���� � ���� �����
			return 0;
		}
	}

	return 1;
}

int mobs_in_room(int m_num, int r_num)
{
	CHAR_DATA *ch;
	int count = 0;

	for (ch = world[r_num]->people; ch; ch = ch->next_in_room)
		if (m_num == GET_MOB_RNUM(ch) && !MOB_FLAGGED(ch, MOB_RESURRECTED))
			count++;

	return count;
}


/*************************************************************************
*  stuff related to the save/load player system                          *
*************************************************************************/

long cmp_ptable_by_name(char *name, int len)
{
	int i;

	len = MIN(len, static_cast<int>(strlen(name)));
	one_argument(name, arg);
	/* Anton Gorev (2015/12/29): I am not sure but I guess that linear search is not the best solution here. TODO: make map helper (MAPHELPER). */
	for (i = 0; i <= top_of_p_table; i++)
	{
		const char* pname = player_table[i].name;
		if (!strn_cmp(pname, arg, MIN(len, static_cast<int>(strlen(pname)))))
		{
			return i;
		}
	}
	return -1;
}



long get_ptable_by_name(const char *name)
{
	int i;

	one_argument(name, arg);
	/* Anton Gorev (2015/12/29): see (MAPHELPER) comment. */
	for (i = 0; i <= top_of_p_table; i++)
	{
		const char* pname = player_table[i].name;
		if (!str_cmp(pname, arg))
		{
			return (i);
		}
	}
	sprintf(buf, "Char %s(%s) not found !!!", name, arg);
	mudlog(buf, LGH, LVL_IMMORT, SYSLOG, FALSE);
	return (-1);
}

long get_ptable_by_unique(long unique)
{
	/* Anton Gorev (2015/12/29): see (MAPHELPER) comment. */
	for (int i = 0; i <= top_of_p_table; i++)
	{
		if (player_table[i].unique == unique)
		{
			return i;
		}
	}
	return 0;
}


long get_id_by_name(char *name)
{
	int i;

	one_argument(name, arg);
	/* Anton Gorev (2015/12/29): see (MAPHELPER) comment. */
	for (i = 0; i <= top_of_p_table; i++)
	{
		if (!str_cmp(player_table[i].name, arg))
		{
			return (player_table[i].id);
		}
	}

	return (-1);
}

long get_id_by_uid(long uid)
{
	/* Anton Gorev (2015/12/29): see (MAPHELPER) comment. */
	for (int i = 0; i <= top_of_p_table; i++)
	{
		if (player_table[i].unique == uid)
		{
			return player_table[i].id;
		}
	}
	return -1;
}

int get_uid_by_id(int id)
{
	/* Anton Gorev (2015/12/29): see (MAPHELPER) comment. */
	for (int i = 0; i <= top_of_p_table; i++)
	{
		if (player_table[i].id == id)
		{
			return player_table[i].unique;
		}
	}
	return -1;
}

const char *get_name_by_id(long id)
{
	/* Anton Gorev (2015/12/29): see (MAPHELPER) comment. */
	for (int i = 0; i <= top_of_p_table; i++)
	{
		if (player_table[i].id == id)
		{
			return player_table[i].name;
		}
	}
	return "";
}

char* get_name_by_unique(int unique)
{
	/* Anton Gorev (2015/12/29): see (MAPHELPER) comment. */
	for (int i = 0; i <= top_of_p_table; i++)
	{
		if (player_table[i].unique == unique)
		{
			return player_table[i].name;
		}
	}
	return 0;
}

int get_level_by_unique(long unique)
{
	int level = 0;

	/* Anton Gorev (2015/12/29): see (MAPHELPER) comment. */
	for (int i = 0; i <= top_of_p_table; ++i)
	{
		if (player_table[i].unique == unique)
		{
			level = player_table[i].level;
		}
	}
	return level;
}

long get_lastlogon_by_unique(long unique)
{
	long time = 0;

	/* Anton Gorev (2015/12/29): see (MAPHELPER) comment. */
	for (int i = 0; i <= top_of_p_table; ++i)
	{
		if (player_table[i].unique == unique)
		{
			time = player_table[i].last_logon;
		}
	}
	return time;
}

int correct_unique(int unique)
{
	/* Anton Gorev (2015/12/29): see (MAPHELPER) comment. */
	for (int i = 0; i <= top_of_p_table; i++)
	{
		if (player_table[i].unique == unique)
		{
			return TRUE;
		}
	}

	return FALSE;
}

int create_unique(void)
{
	int unique;

	do
	{
		unique = (number(0, 64) << 24) + (number(0, 255) << 16) + (number(0, 255) << 8) + (number(0, 255));
	}
	while (correct_unique(unique));
	return (unique);
}

void recreate_saveinfo(const size_t number)
{
	delete player_table[number].timer;
	NEWCREATE(player_table[number].timer);
}

/**
* ����� ����� ������ � get_skill ����� ���������� 100 ��� ��� 200, �� ��� ���� �����
* ��������� ��-���� � ������������ �������� ����� �� ����� ���� �����.
*/
void set_god_skills(CHAR_DATA *ch)
{
	for (const auto i : AVAILABLE_SKILLS)
	{
		ch->set_skill(i, 200);
	}
}

#define NUM_OF_SAVE_THROWS	5

// �� ��������� reboot = 0 (���������� ������ ��� ������)
int load_char(const char *name, CHAR_DATA * char_element, bool reboot)
{
	int player_i;

	player_i = char_element->load_char_ascii(name, reboot);
	if (player_i > -1)
	{
		char_element->set_pfilepos(player_i);
	}
	return (player_i);
}


/*
 * Create a new entry in the in-memory index table for the player file.
 * If the name already exists, by overwriting a deleted character, then
 * we re-use the old position.
 */
int create_entry(const char *name)
{
	int i, pos;

	if (top_of_p_table == -1)  	// no table
	{
		CREATE(player_table, 1);
		pos = top_of_p_table = 0;
	}
	else if ((pos = get_ptable_by_name(name)) == -1)  	// new name
	{
		i = ++top_of_p_table + 1;
		RECREATE(player_table, i);
		pos = top_of_p_table;
	}

	CREATE(player_table[pos].name, strlen(name) + 1);

	// copy lowercase equivalent of name to table field
	for (i = 0, player_table[pos].name[i] = '\0'; (player_table[pos].name[i] = LOWER(name[i])); i++);
	// create new save activity
	player_table[pos].activity = number(0, OBJECT_SAVE_ACTIVITY - 1);
	player_table[pos].timer = NULL;
	player_table[pos].unique = -1;

	return (pos);
}



/************************************************************************
*  funcs of a (more or less) general utility nature                     *
************************************************************************/

// release memory allocated for an obj struct
void free_obj(OBJ_DATA* obj)
{
	obj->purge();
}

/*
 * Steps:
 *   1: Make sure no one is using the pointer in paging.
 *   2: Read contents of a text file.
 *   3: Allocate space.
 *   4: Point 'buf' to it.
 *
 * We don't want to free() the string that someone may be
 * viewing in the pager.  page_string() keeps the internal
 * str_dup()'d copy on ->showstr_head and it won't care
 * if we delete the original.  Otherwise, strings are kept
 * on ->showstr_vector but we'll only match if the pointer
 * is to the string we're interested in and not a copy.
 */
int file_to_string_alloc(const char *name, char **buf)
{
	char temp[MAX_EXTEND_LENGTH];
	DESCRIPTOR_DATA *in_use;

	for (in_use = descriptor_list; in_use; in_use = in_use->next)
	{
		if (in_use->showstr_vector && *in_use->showstr_vector == *buf)
		{
			return (-1);
		}
	}

	// Lets not free() what used to be there unless we succeeded.
	if (file_to_string(name, temp) < 0)
		return (-1);

	if (*buf)
		free(*buf);

	*buf = str_dup(temp);
	return (0);
}


// read contents of a text file, and place in buf
int file_to_string(const char *name, char *buf)
{
	FILE *fl;
	char tmp[READ_SIZE + 3];

	*buf = '\0';

	if (!(fl = fopen(name, "r")))
	{
		log("SYSERR: reading %s: %s", name, strerror(errno));
		sprintf(buf+strlen(buf), "Error: file '%s' is empty.\r\n", name);
		return (0);
	}
	do
	{
		const char* dummy = fgets(tmp, READ_SIZE, fl);
		UNUSED_ARG(dummy);

		tmp[strlen(tmp) - 1] = '\0';	// take off the trailing \n
		strcat(tmp, "\r\n");

		if (!feof(fl))
		{
			if (strlen(buf) + strlen(tmp) + 1 > MAX_EXTEND_LENGTH)
			{
				log("SYSERR: %s: string too big (%d max)", name, MAX_STRING_LENGTH);
				*buf = '\0';
				return (-1);
			}
			strcat(buf, tmp);
		}
	}
	while (!feof(fl));

	fclose(fl);

	return (0);
}

void clear_char_skills(CHAR_DATA * ch)
{
	int i;
	ch->real_abils.Feats.reset();
	for (i = 0; i < MAX_SPELLS + 1; i++)
		ch->real_abils.SplKnw[i] = 0;
	for (i = 0; i < MAX_SPELLS + 1; i++)
		ch->real_abils.SplMem[i] = 0;
	ch->clear_skills();
}

// initialize a new character only if class is set
void init_char(CHAR_DATA * ch)
{
	int i;

#ifdef TEST_BUILD
	if (top_of_p_table == 0)
	{
		// ��� ��������� ����� make test ������ ��� � ���� ���������� ����� 34
		ch->set_level(LVL_IMPL);
	}
#endif

	GET_PORTALS(ch) = NULL;
	CREATE(GET_LOGS(ch), 1 + LAST_LOG);
	ch->set_npc_name(0);
	ch->player_data.long_descr = NULL;
	ch->player_data.description = NULL;
	ch->player_data.time.birth = time(0);
	ch->player_data.time.played = 0;
	ch->player_data.time.logon = time(0);

	// make favors for sex
	if (ch->player_data.sex == ESex::SEX_MALE)
	{
		ch->player_data.weight = number(120, 180);
		ch->player_data.height = number(160, 200);
	}
	else
	{
		ch->player_data.weight = number(100, 160);
		ch->player_data.height = number(150, 180);
	}

	ch->points.hit = GET_MAX_HIT(ch);
	ch->points.max_move = 82;
	ch->points.move = GET_MAX_MOVE(ch);
	ch->real_abils.armor = 100;

	if ((i = get_ptable_by_name(GET_NAME(ch))) != -1)
	{
		ch->set_idnum(++top_idnum);
		player_table[i].id = ch->get_idnum();
		ch->set_uid(create_unique());
		player_table[i].unique = ch->get_uid();
		player_table[i].level = 0;
		player_table[i].last_logon = -1;
		player_table[i].mail = NULL;//added by WorM mail
		player_table[i].last_ip = NULL;//added by WorM ��������� ����
	}
	else
	{
		log("SYSERR: init_char: Character '%s' not found in player table.", GET_NAME(ch));
	}

	if (GET_LEVEL(ch) > LVL_GOD)
	{
		set_god_skills(ch);
		set_god_morphs(ch);
	}

	for (i = 1; i <= MAX_SPELLS; i++)
	{
		if (GET_LEVEL(ch) < LVL_GRGOD)
			GET_SPELL_TYPE(ch, i) = 0;
		else
			GET_SPELL_TYPE(ch, i) = SPELL_KNOW;
	}

	ch->char_specials.saved.affected_by = clear_flags;
	for (i = 0; i < SAVING_COUNT; i++)
		GET_SAVE(ch, i) = 0;
	for (i = 0; i < MAX_NUMBER_RESISTANCE; i++)
		GET_RESIST(ch, i) = 0;

	if (GET_LEVEL(ch) == LVL_IMPL)
	{
		ch->set_str(25);
		ch->set_int(25);
		ch->set_wis(25);
		ch->set_dex(25);
		ch->set_con(25);
		ch->set_cha(25);
	}
	ch->real_abils.size = 50;

	for (i = 0; i < 3; i++)
	{
		GET_COND(ch, i) = (GET_LEVEL(ch) == LVL_IMPL ? -1 : i == DRUNK ? 0 : 24);
	}
	GET_LASTIP(ch)[0] = 0;
//	GET_LOADROOM(ch) = start_room;
	PRF_FLAGS(ch).set(PRF_DISPHP);
	PRF_FLAGS(ch).set(PRF_DISPMANA);
	PRF_FLAGS(ch).set(PRF_DISPEXITS);
	PRF_FLAGS(ch).set(PRF_DISPMOVE);
	PRF_FLAGS(ch).set(PRF_DISPEXP);
	PRF_FLAGS(ch).set(PRF_DISPFIGHT);
	PRF_FLAGS(ch).unset(PRF_SUMMONABLE);
	STRING_LENGTH(ch) = 80;
	STRING_WIDTH(ch) = 30;
	NOTIFY_EXCH_PRICE(ch) = 0;

	ch->save_char();
}

const char *remort_msg =
	"  ���� �� ��� ���������� � ������� ������ ��� ������ -\r\n" "�������� <���������������> ���������.\r\n";

void do_remort(CHAR_DATA *ch, char *argument, int/* cmd*/, int subcmd)
{
	int i, place_of_destination,load_room = NOWHERE;
	const char *remort_msg2 = "$n ��������$g ������������� �������� � ������$g!\r\n";


	if (IS_NPC(ch) || IS_IMMORTAL(ch))
	{
		send_to_char("��� ���, ������, ������ �� � ����.\r\n", ch);
		return;
	}
	if (GET_EXP(ch) < level_exp(ch, LVL_IMMORT) - 1)
	{
		send_to_char("����???\r\n", ch);
		return;
	}
	if (Remort::need_torc(ch) && !PRF_FLAGGED(ch, PRF_CAN_REMORT))
	{
		send_to_char(ch,
			"�� ������ ����������� ���� �������, ����������� ����� ����������� ���������� ������.\r\n"
			"%s\r\n", Remort::WHERE_TO_REMORT_STR.c_str(), ch);
		return;
	}
	if (RENTABLE(ch))
	{
		send_to_char("�� �� ������ ��������������� � ����� � ������� ����������.\r\n", ch);
		return;
	}
	if (!subcmd)
	{
		send_to_char(remort_msg, ch);
		return;
	}

    one_argument(argument, arg);
    if (!*arg)
    {
        sprintf(buf, "�������, ��� �� ������ ������ ������ ���� ����:\r\n");
        sprintf(buf+strlen(buf), "%s", string(BirthPlace::ShowMenu(PlayerRace::GetRaceBirthPlaces(GET_KIN(ch),GET_RACE(ch)))).c_str());
        send_to_char(buf, ch);
        return;
    } else {
        // ������� �������� �� ������ - ����� ��� ������� �������?
        place_of_destination = BirthPlace::ParseSelect(arg);
        if (place_of_destination == BIRTH_PLACE_UNDEFINED)
        {
            //���, ������ ��� ������ � ���������, ��� ������, �������
            place_of_destination = PlayerRace::CheckBirthPlace(GET_KIN(ch), GET_RACE(ch), arg);
            if (!BirthPlace::CheckId(place_of_destination))
            {
                send_to_char("������ ������, �������� ���� �������� ����� ������ ����.\r\n", ch);
                return;
            }
        }
    }

	log("Remort %s", GET_NAME(ch));
	ch->remort();
	act(remort_msg2, FALSE, ch, 0, 0, TO_ROOM);

	if (ch->is_morphed()) ch->reset_morph();
	ch->set_remort(ch->get_remort() + 1);
	CLR_GOD_FLAG(ch, GF_REMORT);
	ch->inc_str(1);
	ch->inc_dex(1);
	ch->inc_con(1);
	ch->inc_wis(1);
	ch->inc_int(1);
	ch->inc_cha(1);

	if (ch->get_fighting())
		stop_fighting(ch, TRUE);

	die_follower(ch);

	while (ch->helpers)
	{
		REMOVE_FROM_LIST(ch->helpers, ch->helpers, [](auto list) -> auto& { return list->next_helper; });
	}

	while (!ch->affected.empty())
	{
		ch->affect_remove(ch->affected.begin());
	}

// ������� ���� �����
	for (i = 0; i < NUM_WEARS; i++)
	{
		if (GET_EQ(ch, i))
		{
			obj_to_char(unequip_char(ch, i), ch);
		}
	}

	while (ch->timed)
	{
		timed_from_char(ch, ch->timed);
	}
	

	

	while (ch->timed_feat)
	{
		timed_feat_from_char(ch, ch->timed_feat);
	}
	for (i = 1; i < MAX_FEATS; i++)
	{
		UNSET_FEAT(ch, i);
	}
	
	if (ch->get_remort() >= 9 && ch->get_remort() % 3 == 0)
	{
		ch->clear_skills();
		for (i = 1; i <= MAX_SPELLS; i++)
		{
			GET_SPELL_TYPE(ch, i) = (GET_CLASS(ch) == CLASS_DRUID ? SPELL_RUNES : 0);
			GET_SPELL_MEM(ch, i) = 0;
		}
	}
	else
	{
		ch->crop_skills();
		for (i = 1; i <= MAX_SPELLS; i++)
		{
			if (GET_CLASS(ch) == CLASS_DRUID)
			{
				GET_SPELL_TYPE(ch, i) = SPELL_RUNES;
			}
			else if (spell_info[i].slot_forc[(int) GET_CLASS(ch)][(int) GET_KIN(ch)] >= 8)
			{
				GET_SPELL_TYPE(ch, i) = 0;
				GET_SPELL_MEM(ch, i) = 0;
			}			
		}
	}

	GET_HIT(ch) = GET_MAX_HIT(ch) = 10;
	GET_MOVE(ch) = GET_MAX_MOVE(ch) = 82;
	GET_MEM_TOTAL(ch) = GET_MEM_COMPLETED(ch) = 0;
	ch->set_level(0);
	GET_WIMP_LEV(ch) = 0;
	GET_AC(ch) = 100;
	GET_LOADROOM(ch) = calc_loadroom(ch, place_of_destination);
	PRF_FLAGS(ch).unset(PRF_SUMMONABLE);
	PRF_FLAGS(ch).unset(PRF_AWAKE);
	PRF_FLAGS(ch).unset(PRF_PUNCTUAL);
	PRF_FLAGS(ch).unset(PRF_POWERATTACK);
	PRF_FLAGS(ch).unset(PRF_GREATPOWERATTACK);
	PRF_FLAGS(ch).unset(PRF_AWAKE);
	PRF_FLAGS(ch).unset(PRF_IRON_WIND);
	// ������� ��� ��������� �������
	check_portals(ch);
	if (ch->get_protecting())
	{
		ch->set_protecting(0);
		ch->BattleAffects.unset(EAF_PROTECT);
	}
	
	//��������� ���������� ����� ��� �������� ��������������
	GET_RIP_DTTHIS(ch) = 0;
	GET_EXP_DTTHIS(ch) = 0;
	GET_RIP_MOBTHIS(ch) = 0;
	GET_EXP_MOBTHIS(ch) = 0;
	GET_RIP_PKTHIS(ch) = 0;
	GET_EXP_PKTHIS(ch) = 0;
	GET_RIP_OTHERTHIS(ch) = 0;
	GET_EXP_OTHERTHIS(ch) = 0;

	do_start(ch, FALSE);
	ch->save_char();
	if (PLR_FLAGGED(ch, PLR_HELLED))
		load_room = r_helled_start_room;
	else if (PLR_FLAGGED(ch, PLR_NAMED))
		load_room = r_named_start_room;
	else if (PLR_FLAGGED(ch, PLR_FROZEN))
		load_room = r_frozen_start_room;
	else
	{
		if ((load_room = GET_LOADROOM(ch)) == NOWHERE)
			load_room = calc_loadroom(ch);
		load_room = real_room(load_room);
	}
	if (load_room == NOWHERE)
	{
		if (GET_LEVEL(ch) >= LVL_IMMORT)
			load_room = r_immort_start_room;
		else
			load_room = r_mortal_start_room;
	}
	char_from_room(ch);
	char_to_room(ch, load_room);
	look_at_room(ch, 0);
	PLR_FLAGS(ch).set(PLR_NODELETE);
	remove_rune_label(ch);

	// ����� �����, ���������� � �������� (������ ���������)
	PRF_FLAGS(ch).unset(PRF_CAN_REMORT);
	ch->set_ext_money(ExtMoney::TORC_GOLD, 0);
	ch->set_ext_money(ExtMoney::TORC_SILVER, 0);
	ch->set_ext_money(ExtMoney::TORC_BRONZE, 0);

	snprintf(buf, sizeof(buf),
		"remort from %d to %d", ch->get_remort() - 1, ch->get_remort());
	snprintf(buf2, sizeof(buf2), "dest=%d", place_of_destination);
	add_karma(ch, buf, buf2);

	act("$n �������$g � ����.", TRUE, ch, 0, 0, TO_ROOM);
	act("�� ���������������! ������ �����!", FALSE, ch, 0, 0, TO_CHAR);
}

// returns the real number of the room with given virtual number
room_rnum real_room(room_vnum vnum)
{
	room_rnum bot, top, mid;

	bot = 1;		// 0 - room is NOWHERE
	top = top_of_world;
	// perform binary search on world-table
	for (;;)
	{
		mid = (bot + top) / 2;

		if (world[mid]->number == vnum)
			return (mid);
		if (bot >= top)
			return (NOWHERE);
		if (world[mid]->number > vnum)
			top = mid - 1;
		else
			bot = mid + 1;
	}
}

// returns the real number of the monster with given virtual number
mob_rnum real_mobile(mob_vnum vnum)
{
	mob_rnum bot, top, mid;

	bot = 0;
	top = top_of_mobt;

	// perform binary search on mob-table
	for (;;)
	{
		mid = (bot + top) / 2;

		if ((mob_index + mid)->vnum == vnum)
			return (mid);
		if (bot >= top)
			return (-1);
		if ((mob_index + mid)->vnum > vnum)
			top = mid - 1;
		else
			bot = mid + 1;
	}
}

// ������ ������� �������� � ����������� ����������� ����������
// ����������� � ����������� � load_char_ascii
int must_be_deleted(CHAR_DATA * short_ch)
{
	int ci, timeout;

	if (PLR_FLAGS(short_ch).get(PLR_NODELETE))
	{
		return 0;
	}

	if (GET_REMORT(short_ch))
		return (0);

	if (PLR_FLAGS(short_ch).get(PLR_DELETED))
	{
		return 1;
	}

	timeout = -1;
	for (ci = 0; ci == 0 || pclean_criteria[ci].level > pclean_criteria[ci - 1].level; ci++)
	{
		if (GET_LEVEL(short_ch) <= pclean_criteria[ci].level)
		{
			timeout = pclean_criteria[ci].days;
			break;
		}
	}
	if (timeout >= 0)
	{
		timeout *= SECS_PER_REAL_DAY;
		if ((time(0) - LAST_LOGON(short_ch)) > timeout)
		{
			return (1);
		}
	}

	return (0);
}

// ������ ������� �������� � ����������� ����������� ����������
// ����������� � ����������� � load_char_ascii
void entrycount(char *name)
{
	int i, deleted;
	char filename[MAX_STRING_LENGTH];

	if (get_filename(name, filename, PLAYERS_FILE))
	{
		Player t_short_ch;
		Player *short_ch = &t_short_ch;
		deleted = 1;
		// �������� ����������� �����������
		if (load_char(name, short_ch, 1) > -1)
		{
			// ���� ��� ������ ��� �� ����� �� �������, �� �� ������� ��� ���� ������
			if (!must_be_deleted(short_ch))
			{
				deleted = 0;
				// new record
				if (player_table)
					RECREATE(player_table, top_of_p_table + 2);
				else
					CREATE(player_table, 1);
				top_of_p_file++;
				top_of_p_table++;

				CREATE(player_table[top_of_p_table].name, strlen(GET_NAME(short_ch)) + 1);
				for (i = 0, player_table[top_of_p_table].name[i] = '\0';
						(player_table[top_of_p_table].name[i] = LOWER(GET_NAME(short_ch)[i])); i++);
				//added by WorM 2010.08.27 � ������� ������ ���� � ip
				CREATE(player_table[top_of_p_table].mail, strlen(GET_EMAIL(short_ch)) + 1);
				for (i = 0, player_table[top_of_p_table].mail[i] = '\0';
						(player_table[top_of_p_table].mail[i] = LOWER(GET_EMAIL(short_ch)[i])); i++);
				CREATE(player_table[top_of_p_table].last_ip, strlen(GET_LASTIP(short_ch)) + 1);
				for (i = 0, player_table[top_of_p_table].last_ip[i] = '\0';
						(player_table[top_of_p_table].last_ip[i] = GET_LASTIP(short_ch)[i]); i++);
				//end by WorM
				player_table[top_of_p_table].id = GET_IDNUM(short_ch);
				player_table[top_of_p_table].unique = GET_UNIQUE(short_ch);
				player_table[top_of_p_table].level = (GET_REMORT(short_ch) && !IS_IMMORTAL(short_ch)) ? 30 : GET_LEVEL(short_ch);
				player_table[top_of_p_table].timer = NULL;
				if (PLR_FLAGS(short_ch).get(PLR_DELETED))
				{
					player_table[top_of_p_table].last_logon = -1;
					player_table[top_of_p_table].activity = -1;
					/*//added by WorM 2010.08.27 � ������� ������ ���� � ip
					if(player_table[top_of_p_table].mail)
					{
						free(player_table[top_of_p_table].mail);
					}
					player_table[top_of_p_table].mail = NULL;
					if(player_table[top_of_p_table].last_ip)
					{
						free(player_table[top_of_p_table].last_ip);
					}
					player_table[top_of_p_table].last_ip = NULL;
					//end by WorM*/
				}
				else
				{
					player_table[top_of_p_table].last_logon = LAST_LOGON(short_ch);
					player_table[top_of_p_table].activity = number(0, OBJECT_SAVE_ACTIVITY - 1);
				}
				#ifdef TEST_BUILD
				log("entry: char:%s level:%d mail:%s ip:%s", player_table[top_of_p_table].name, player_table[top_of_p_table].level, player_table[top_of_p_table].mail, player_table[top_of_p_table].last_ip);
				#endif
				top_idnum = MAX(top_idnum, GET_IDNUM(short_ch));
				TopPlayer::Refresh(short_ch, 1);
				log("Add new player %s", player_table[top_of_p_table].name);
			}
		}
		// ���� ��� ��� ������, �� ������� � ����� ��� ����
		if (deleted)
		{
			log("Player %s already deleted - kill player file", name);
			remove(filename);
			// 2) Remove all other files
			get_filename(name, filename, ALIAS_FILE);
			remove(filename);
			get_filename(name, filename, SCRIPT_VARS_FILE);
			remove(filename);
			get_filename(name, filename, PERS_DEPOT_FILE);
			remove(filename);
			get_filename(name, filename, SHARE_DEPOT_FILE);
			remove(filename);
			get_filename(name, filename, PURGE_DEPOT_FILE);
			remove(filename);
			get_filename(name, filename, TEXT_CRASH_FILE);
			remove(filename);
			get_filename(name, filename, TIME_CRASH_FILE);
			remove(filename);
		}
	}
	return;
}

void new_build_player_index(void)
{
	FILE *players;
	char name[MAX_INPUT_LENGTH], playername[MAX_INPUT_LENGTH];
	int c;

	player_table = NULL;
	top_of_p_file = top_of_p_table = -1;
	if (!(players = fopen(LIB_PLRS "players.lst", "r")))
	{
		log("Players list empty...");
		return;
	}

	now_entrycount = TRUE;
	while (get_line(players, name))
	{
		if (!*name || *name == ';')
			continue;
		if (sscanf(name, "%s ", playername) == 0)
			continue;
		for (c = 0; c <= top_of_p_table; c++)
			if (!str_cmp(playername, player_table[c].name))
				break;
		if (c <= top_of_p_table)
			continue;
		entrycount(playername);
	}
	fclose(players);
	now_entrycount = FALSE;
}

void flush_player_index(void)
{
	FILE *players;
	char name[MAX_STRING_LENGTH];
	int i;

	if (!(players = fopen(LIB_PLRS "players.lst", "w+")))
	{
		log("Cann't save players list...");
		return;
	}
	for (i = 0; i <= top_of_p_table; i++)
	{
		if (!player_table[i].name || !*player_table[i].name)
			continue;

		// check double
		// for (c = 0; c < i; c++)
		//     if (!str_cmp(player_table[c].name, player_table[i].name))
		//         break;
		// if (c < i)
		//    continue;

		sprintf(name, "%s %d %d %d %d\n",
				player_table[i].name,
				player_table[i].id, player_table[i].unique, player_table[i].level, player_table[i].last_logon);
		fputs(name, players);
	}
	fclose(players);
	log("��������� �������� %d (������� ��� �������� %d)", i, top_of_p_file + 1);
}

void dupe_player_index(void)
{
	FILE *players;
	char name[MAX_STRING_LENGTH];
	int i, c;

	sprintf(name, LIB_PLRS "players.dup");

	if (!(players = fopen(name, "w+")))
	{
		log("Cann't save players list...");
		return;
	}
	for (i = 0; i <= top_of_p_table; i++)
	{
		if (!player_table[i].name || !*player_table[i].name)
			continue;

		// check double
		for (c = 0; c < i; c++)
			if (!str_cmp(player_table[c].name, player_table[i].name))
				break;
		if (c < i)
			continue;

		sprintf(name, "%s %d %d %d %d\n",
				player_table[i].name,
				player_table[i].id, player_table[i].unique, player_table[i].level, player_table[i].last_logon);
		fputs(name, players);
	}
	fclose(players);
	log("�������������� �������� %d (������� ��� �������� %d)", i, top_of_p_file + 1);
}

void rename_char(CHAR_DATA * ch, char *oname)
{
	char filename[MAX_INPUT_LENGTH], ofilename[MAX_INPUT_LENGTH];

	// 1) Rename(if need) char and pkill file - directly
	log("Rename char %s->%s", GET_NAME(ch), oname);
	get_filename(oname, ofilename, PLAYERS_FILE);
	get_filename(GET_NAME(ch), filename, PLAYERS_FILE);
	rename(ofilename, filename);

	ch->save_char();

	// 2) Rename all other files
	get_filename(oname, ofilename, TEXT_CRASH_FILE);
	get_filename(GET_NAME(ch), filename, TEXT_CRASH_FILE);
	rename(ofilename, filename);

	get_filename(oname, ofilename, TIME_CRASH_FILE);
	get_filename(GET_NAME(ch), filename, TIME_CRASH_FILE);
	rename(ofilename, filename);

	get_filename(oname, ofilename, ALIAS_FILE);
	get_filename(GET_NAME(ch), filename, ALIAS_FILE);
	rename(ofilename, filename);

	get_filename(oname, ofilename, SCRIPT_VARS_FILE);
	get_filename(GET_NAME(ch), filename, SCRIPT_VARS_FILE);
	rename(ofilename, filename);

	// ���������
	Depot::rename_char(ch);
	get_filename(oname, ofilename, PERS_DEPOT_FILE);
	get_filename(GET_NAME(ch), filename, PERS_DEPOT_FILE);
	rename(ofilename, filename);
	get_filename(oname, ofilename, PURGE_DEPOT_FILE);
	get_filename(GET_NAME(ch), filename, PURGE_DEPOT_FILE);
	rename(ofilename, filename);
}

// * ������������ �������� ��������� ����� ������� ����.
void delete_char(const char *name)
{
	Player t_st;
	Player *st = &t_st;
	int id = load_char(name, st);

	if (id >= 0)
	{
		PLR_FLAGS(st).set(PLR_DELETED);
		NewNameRemove(st);
		Clan::remove_from_clan(GET_UNIQUE(st));
		st->save_char();

		Crash_clear_objects(id);
		player_table[id].unique = -1;
		player_table[id].level = -1;
		player_table[id].last_logon = -1;
		player_table[id].activity = -1;
		if(player_table[id].mail)
			free(player_table[id].mail);
		player_table[id].mail = NULL;
		if(player_table[id].last_ip)
			free(player_table[id].last_ip);
		player_table[id].last_ip = NULL;
	}
}

void room_copy(ROOM_DATA * dst, ROOM_DATA * src)
/*++
   ������� ������ ������� ����� �������.
   ����� ������ ���� ������� ��������� ��������� ����������� ����� ������ src
   �� ����������� ����� track, contents, people, affected.
   ��� ���� ����� �� �� ��������, �� �������� ���� ������� ������.
      dst - "������" ��������� �� ��������� room_data.
      src - �������� �������
   ����������: ����������� ��������� dst �������� � ������ ������.
               ����������� redit_room_free() ��� ������� ����������� �������
--*/
{
	int i;
	{
		// �������� track, contents, people, �������
		struct track_data *track = dst->track;
		OBJ_DATA *contents = dst->contents;
		CHAR_DATA *people = dst->people;
		auto affected = dst->affected;

		// ������� ��� ������
		*dst = *src;

		// �������������� track, contents, people, �������
		dst->track = track;
		dst->contents = contents;
		dst->people = people;
		dst->affected = affected;
	}

	// ������ ����� �������� ����������� ������� ������

	// �������� � ��������
	dst->name = str_dup(src->name ? src->name : "������������");
	dst->temp_description = 0; // ��� ����

	// ������ � �����
	for (i = 0; i < NUM_OF_DIRS; ++i)
	{
		const auto& rdd = src->dir_option[i];
		if (rdd)
		{
			dst->dir_option[i].reset(new EXIT_DATA());
			// �������� �����
			*dst->dir_option[i] = *rdd;
			// �������� ������
			dst->dir_option[i]->general_description = rdd->general_description;
			dst->dir_option[i]->keyword = (rdd->keyword ? str_dup(rdd->keyword) : NULL);
			dst->dir_option[i]->vkeyword = (rdd->vkeyword ? str_dup(rdd->vkeyword) : NULL);
		}
	}

	// �������������� ��������, ���� ����
	EXTRA_DESCR_DATA::shared_ptr* pddd = &dst->ex_description;
	EXTRA_DESCR_DATA::shared_ptr sdd = src->ex_description;
	*pddd = nullptr;

	while (sdd)
	{
		pddd->reset(new EXTRA_DESCR_DATA());
		(*pddd)->keyword = sdd->keyword ? str_dup(sdd->keyword) : nullptr;
		(*pddd)->description = sdd->description ? str_dup(sdd->description) : nullptr;
		pddd = &((*pddd)->next);
		sdd = sdd->next;
	}

	// ������� ������ � ���������
	SCRIPT(dst) = nullptr;
	dst->proto_script.reset(new OBJ_DATA::triggers_list_t());
	*dst->proto_script = *src->proto_script;

	im_inglist_copy(&dst->ing_list, src->ing_list);
}

void free_script(SCRIPT_DATA * sc);

void room_free(ROOM_DATA * room)
/*++
   ������� ��������� ����������� ������, ���������� ������� �������.
   ��������. ������ ����� ��������� room_data �� �������������.
             ���������� ������������� ������������ delete()
--*/
{
	int i;

	// �������� � ��������
	if (room->name)
		free(room->name);
	if (room->temp_description)
	{
		free(room->temp_description);
		room->temp_description = 0;
	}

	// ������ � �����
	for (i = 0; i < NUM_OF_DIRS; i++)
	{
		if (room->dir_option[i])
		{
			if (room->dir_option[i]->keyword)
				free(room->dir_option[i]->keyword);
			if (room->dir_option[i]->vkeyword)
				free(room->dir_option[i]->vkeyword);
			room->dir_option[i].reset();
		}
	}

	// ������
	free_script(SCRIPT(room));

	if (room->ing_list)
	{
		free(room->ing_list);
		room->ing_list = NULL;
	}

	room->affected.clear();
}

void LoadGlobalUID(void)
{
	FILE *guid;
	char buffer[256];

	global_uid = 0;

	if (!(guid = fopen(LIB_MISC "globaluid", "r")))
	{
		log("Can't open global uid file...");
		return;
	}
	get_line(guid, buffer);
	global_uid = atoi(buffer);
	fclose(guid);
	return;
}

void SaveGlobalUID(void)
{
	FILE *guid;

	if (!(guid = fopen(LIB_MISC "globaluid", "w")))
	{
		log("Can't write global uid file...");
		return;
	}

	fprintf(guid, "%d\n", global_uid);
	fclose(guid);
	return;
}

void load_guardians()
{
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(LIB_MISC"guards.xml");
	if (!result)
	{
		snprintf(buf, MAX_STRING_LENGTH, "...%s", result.description());
		mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
		return;
	}

	pugi::xml_node xMainNode = doc.child("guardians");

	if (!xMainNode)
	{
		snprintf(buf, MAX_STRING_LENGTH, "...guards.xml read fail");
		mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
		return;
	}

	guardian_list.clear();

	int num_wars_global = atoi(xMainNode.child_value("wars"));

	struct mob_guardian tmp_guard;
	for (pugi::xml_node xNodeGuard = xMainNode.child("guard");xNodeGuard; xNodeGuard = xNodeGuard.next_sibling("guard"))
	{
		int guard_vnum = xNodeGuard.attribute("vnum").as_int();

		if (guard_vnum <= 0)
		{
			log("ERROR: ������ �������� ����� %s - ������������ �������� VNUM: %d", LIB_MISC"guards.xml", guard_vnum);
			continue;
		}
		//�������� �� ���������
		tmp_guard.max_wars_allow = num_wars_global;
		tmp_guard.agro_all_agressors = false;
		tmp_guard.agro_argressors_in_zones.clear();
		tmp_guard.agro_killers = true;

		int num_wars = xNodeGuard.attribute("wars").as_int();

		if (num_wars && (num_wars != num_wars_global))
			tmp_guard.max_wars_allow = num_wars;

		if (!strcmp(xNodeGuard.attribute("killer").value(),"no"))
			tmp_guard.agro_killers = false;

		if (!strcmp(xNodeGuard.attribute("agressor").value(),"yes"))
			tmp_guard.agro_all_agressors = true;

		if (!strcmp(xNodeGuard.attribute("agressor").value(),"list"))
		{
			for (pugi::xml_node xNodeZone = xNodeGuard.child("zone"); xNodeZone; xNodeZone = xNodeZone.next_sibling("zone"))
			{
				tmp_guard.agro_argressors_in_zones.push_back(atoi(xNodeZone.child_value()));
			}
		}
		guardian_list[guard_vnum] = tmp_guard;
	}

}

//Polud �������� ����� ��� �������� ���������� ��������� ��� �����
//������ ������ �� �����
const char *MOBRACE_FILE = LIB_MISC_MOBRACES "mobrace.xml";

MobRaceListType mobraces_list;

//�������� ��� �� �����
void load_mobraces()
{
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(MOBRACE_FILE);
	if (!result)
	{
		snprintf(buf, MAX_STRING_LENGTH, "...%s", result.description());
		mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
		return;
	}

	pugi::xml_node node_list = doc.child("mobraces");

	if (!node_list)
	{
		snprintf(buf, MAX_STRING_LENGTH, "...mobraces read fail");
		mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
		return;
	}

	for (pugi::xml_node  race = node_list.child("mobrace");race; race = race.next_sibling("mobrace"))
	{
		MobRacePtr tmp_mobrace(new MobRace);
		tmp_mobrace->race_name = race.attribute("name").value();
		int race_num = race.attribute("key").as_int();

		pugi::xml_node imList = race.child("imlist");

		for (pugi::xml_node im = imList.child("im");im;im = im.next_sibling("im"))
		{
			struct ingredient tmp_ingr;
			tmp_ingr.imtype = im.attribute("type").as_int();
			tmp_ingr.imname = string(im.attribute("name").value());
			boost::trim(tmp_ingr.imname);
			int cur_lvl=1;
			int prob_value = 1;
			for (pugi::xml_node prob = im.child("prob"); prob; prob = prob.next_sibling("prob"))
			{
				int next_lvl = prob.attribute("lvl").as_int();
				if (next_lvl>0)
				{
					for (int lvl=cur_lvl; lvl < next_lvl; lvl++)
						tmp_ingr.prob[lvl-1] = prob_value;
				}
				else
				{
					log("SYSERROR: �������� ������� lvl=%d ��� ����������� %s ���� %s", next_lvl, tmp_ingr.imname.c_str(), tmp_mobrace->race_name.c_str());
					return;
				}
				prob_value = atoi(prob.child_value());
				cur_lvl = next_lvl;
			}
			for (int lvl=cur_lvl; lvl <= MAX_MOB_LEVEL; lvl++)
				tmp_ingr.prob[lvl-1] = prob_value;

			tmp_mobrace->ingrlist.push_back(tmp_ingr);
		}
		mobraces_list[race_num] = tmp_mobrace;
	}
}


MobRace::MobRace()
{
	ingrlist.clear();
}

MobRace::~MobRace()
{
	ingrlist.clear();
}
//-Polud

////////////////////////////////////////////////////////////////////////////////
namespace OfftopSystem
{

const char* BLOCK_FILE = LIB_MISC"offtop.lst";
std::vector<std::string> block_list;

/// �������� �� ������� ���� � ����-������ � ��� �����
void set_flag(CHAR_DATA* ch)
{
	std::string mail(GET_EMAIL(ch));
	lower_convert(mail);
	auto i = std::find(block_list.begin(), block_list.end(), mail);
	if (i != block_list.end())
	{
		PRF_FLAGS(ch).set(PRF_IGVA_PRONA);
	}
	else
	{
		PRF_FLAGS(ch).unset(PRF_IGVA_PRONA);
	}
}

/// ����/������ ������ ������������� ��� ������� ����������.
void init()
{
	block_list.clear();
	std::ifstream file(BLOCK_FILE);
	if (!file.is_open())
	{
		log("SYSERROR: �� ������� ������� ���� �� ������: %s", BLOCK_FILE);
		return;
	}
	std::string buffer;
	while (file >> buffer)
	{
		lower_convert(buffer);
		block_list.push_back(buffer);
	}
	for (DESCRIPTOR_DATA* d = descriptor_list; d; d = d->next)
	{
		if (d->character)
		{
			set_flag(d->character);
		}
	}
}

} // namespace OfftopSystem
////////////////////////////////////////////////////////////////////////////////
std::map<int, std::string> daily_array;


int dg_daily_quest(CHAR_DATA *, int, int)
{
	return 0;
}

void load_daily_quest()
{
	pugi::xml_document doc_;
	pugi::xml_node child_, object_, file_;
	file_ = XMLLoad(LIB_MISC DAILY_FILE, "daily_root", "Error loading cases file: daily.xml", doc_);

	for (child_ = file_.child("daily"); child_; child_ = child_.next_sibling("daily"))
	{
		int temp_id = child_.attribute("id").as_int();
		std::string temp_desk = child_.attribute("desk").as_string();
		daily_array.insert(std::pair<int, std::string>(temp_id, temp_desk));
	}

}

void load_speedwalk()
{

	pugi::xml_document doc_sw;
	pugi::xml_node child_, object_, file_sw;
	file_sw = XMLLoad(LIB_MISC SPEEDWALKS_FILE, "speedwalks", "Error loading cases file: speedwalks.xml", doc_sw);

	for (child_ = file_sw.child("speedwalk"); child_; child_ = child_.next_sibling("speedwalk"))
	{
		SpeedWalk sw;
		sw.wait = 0;
		sw.cur_state = 0;
		sw.default_wait = child_.attribute("default_wait").as_int();
		for (object_ = child_.child("mob"); object_; object_ = object_.next_sibling("mob"))
			sw.vnum_mobs.push_back(object_.attribute("vnum").as_int());
		for (object_ = child_.child("route"); object_; object_ = object_.next_sibling("route"))
		{
			Route r;
			r.direction = object_.attribute("direction").as_string();
			r.wait = object_.attribute("wait").as_int();
			sw.route.push_back(r);
		}
		speedwalks.push_back(sw);

	}
	for (CHAR_DATA *ch = character_list; ch; ch = ch->get_next())
	{
		for (auto &sw : speedwalks)
		{
			for (auto mob : sw.vnum_mobs)
			{
				if (GET_MOB_VNUM(ch) == mob)
					sw.mobs.push_back(ch);
			}
		}
	}

}

void load_class_limit()
{
	pugi::xml_document doc_sw;
	pugi::xml_node child_, object_, file_sw;
	
	for (int i = 0; i < NUM_PLAYER_CLASSES; ++i)
	{
		for (int j = 0; j < 6; ++j)
		{
			//Intiializing stats limit with default value
			class_stats_limit[i][j] = 50;
		}
	}

	file_sw = XMLLoad(LIB_MISC CLASS_LIMIT_FILE, "class_limit", "Error loading file: classlimit.xml", doc_sw);

	for (child_ = file_sw.child("stats_limit").child("class"); child_; child_ = child_.next_sibling("class"))
	{
		std::string id_str = Parse::attr_str(child_, "id");
		if (id_str.empty())
			continue;

		const int id = TextId::to_num(TextId::CHAR_CLASS, id_str);
		if (id == CLASS_UNDEFINED)
		{
			snprintf(buf, MAX_STRING_LENGTH, "...<class id='%s'> convert fail", id_str.c_str());
			mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
			continue;
		}

		int val = Parse::child_value_int(child_, "str");
		if (val > 0)
			class_stats_limit[id][0] = val;

		val = Parse::child_value_int(child_, "dex");
		if (val > 0)
			class_stats_limit[id][1] = val;

		val = Parse::child_value_int(child_, "con");
		if (val > 0)
			class_stats_limit[id][2] = val;

		val = Parse::child_value_int(child_, "wis");
		if (val > 0)
			class_stats_limit[id][3] = val;

		val = Parse::child_value_int(child_, "int");
		if (val> 0)
			class_stats_limit[id][4] = val;

		val = Parse::child_value_int(child_, "cha");
		if (val > 0)
			class_stats_limit[id][5] = val;
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
