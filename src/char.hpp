// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2008 Krodo
// Part of Bylins http://www.mud.ru

#ifndef CHAR_HPP_INCLUDED
#define CHAR_HPP_INCLUDED

#include <bitset>
#include <map>
#include <boost/shared_ptr.hpp>
#include <boost/array.hpp>
//#include <ext/hash_map>
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "player_i.hpp"

/* These data contain information about a players time data */
struct time_data
{
	time_t birth;		/* This represents the characters age                */
	time_t logon;		/* Time of the last logon (used to calculate played) */
	int played;		/* This is the total accumulated time played in secs */
};

/* general player-related info, usually PC's and NPC's */
struct char_player_data
{
	char *long_descr;	/* for 'look'               */
	char *description;	/* Extra descriptions                   */
	char *title;		/* PC / NPC's title                     */
	byte sex;		/* PC / NPC's sex                       */
	struct time_data time;			/* PC's AGE in days                 */
	ubyte weight;		/* PC / NPC's weight                    */
	ubyte height;		/* PC / NPC's height                    */

	boost::array<char *, 6> PNames;
	ubyte Religion;
	ubyte Kin;
	ubyte Race;		/* PC / NPC's race*/
};

// ���-�� +������ �� ������
const int MAX_ADD_SLOTS = 10;
// ���� ��������
enum { FIRE_RESISTANCE = 0, AIR_RESISTANCE, WATER_RESISTANCE, EARTH_RESISTANCE, VITALITY_RESISTANCE, MIND_RESISTANCE, IMMUNITY_RESISTANCE, MAX_NUMBER_RESISTANCE };

/* Char's additional abilities. Used only while work */
struct char_played_ability_data
{
	int str_add;
	int intel_add;
	int wis_add;
	int dex_add;
	int cha_add;
	int weight_add;
	int height_add;
	int size_add;
	int age_add;
	int hit_add;
	int move_add;
	int hitreg;
	int movereg;
	int manareg;
	boost::array<sbyte, MAX_ADD_SLOTS> obj_slot;
	int armour;
	int ac_add;
	int hr_add;
	int dr_add;
	int absorb;
	int morale_add;
	int cast_success;
	int initiative_add;
	int poison_add;
	int pray_add;
	boost::array<sh_int, 4> apply_saving_throw;		/* Saving throw (Bonuses)  */
	boost::array<sh_int, MAX_NUMBER_RESISTANCE> apply_resistance_throw;	/* ������������� (�������) � �����, ���� � ����. ������ */
	ubyte mresist;
	ubyte aresist;
	ubyte presist;	// added by WorM(�������) �� ������� <������������> (����������) ������������ ������� ���������� ���.����� � %
};

/* Char's abilities. */
struct char_ability_data
{
	boost::array<ubyte, MAX_SPELLS + 1> SplKnw; /* array of SPELL_KNOW_TYPE         */
	boost::array<ubyte, MAX_SPELLS + 1> SplMem; /* array of MEMed SPELLS            */
	bitset<MAX_FEATS> Feats;
	sbyte str;
	sbyte intel;
	sbyte wis;
	sbyte dex;
	sbyte cha;
	sbyte size;
	sbyte hitroll;
	int damroll;
	short armor;
};

/* Char's points. */
struct char_point_data
{
	int hit;
	int max_hit;		/* Max hit for PC/NPC                      */
	sh_int move;
	sh_int max_move;	/* Max move for PC/NPC                     */
};

/*
 * char_special_data_saved: specials which both a PC and an NPC have in
 * common, but which must be saved to the playerfile for PC's.
 *
 * WARNING:  Do not change this structure.  Doing so will ruin the
 * playerfile.  If you want to add to the playerfile, use the spares
 * in player_special_data.
 */
struct char_special_data_saved
{
	int alignment;		/* +-1000 for alignments                */
	FLAG_DATA act;		/* act flag for NPC's; player flag for PC's */
	FLAG_DATA affected_by;
	/* Bitvector for spells/skills affected by */
};

/* Special playing constants shared by PCs and NPCs which aren't in pfile */
struct char_special_data
{
	byte position;		/* Standing, fighting, sleeping, etc. */

	int carry_weight;		/* Carried weight */
	int carry_items;		/* Number of items carried   */
	int timer;			/* Timer for update  */

	struct char_special_data_saved saved;			/* constants saved in plrfile  */
};

/* Specials used by NPCs, not PCs */
struct mob_special_data
{
	byte last_direction;	/* The last direction the monster went     */
	int attack_type;		/* The Attack Type Bitvector for NPC's     */
	byte default_pos;	/* Default position for NPC                */
	memory_rec *memory;	/* List of attackers to remember          */
	byte damnodice;		/* The number of damage dice's             */
	byte damsizedice;	/* The size of the damage dice's           */
	boost::array<int, MAX_DEST> dest;
	int dest_dir;
	int dest_pos;
	int dest_count;
	int activity;
	FLAG_DATA npc_flags;
	byte ExtraAttack;
	byte LikeWork;
	byte MaxFactor;
	int GoldNoDs;
	int GoldSiDs;
	int HorseState;
	int LastRoom;
	char *Questor;
	int speed;
	int hire_price;// added by WorM (�������) 2010.06.04 ���� ����� �������
};

// ������� ����������� ����������
struct spell_mem_queue
{
	struct spell_mem_queue_item *queue;
	int stored;		// ��������� �����
	int total;			// ������ ����� ���� ���� �������
};

/* Structure used for extra_attack - bash, kick, diasrm, chopoff, etc */
struct extra_attack_type
{
	int used_skill;
	CHAR_DATA *victim;
};

struct cast_attack_type
{
	int spellnum;
	int spell_subst;
	CHAR_DATA *tch;
	OBJ_DATA *tobj;
	ROOM_DATA *troom;
};

struct player_special_data_saved
{
	int wimp_level;		/* Below this # of hit points, flee!  */
	int invis_level;		/* level of invisibility      */
	room_vnum load_room;	/* Which room to place char in      */
	FLAG_DATA pref;		/* preference flags for PC's.    */
	int bad_pws;		/* number of bad password attemps   */
	boost::array<int, 3> conditions;		/* Drunk, full, thirsty        */

	int DrunkState;
	int olc_zone;
	int NameGod;
	long NameIDGod;
	long GodsLike;

	char EMail[128];
	char LastIP[128];

	int stringLength;
	int stringWidth;

	/*29.11.09 ���������� ��� �������� ���������� ����� (�) ��������*/
	int Rip_arena; //���� �� �����
	int Rip_mob; // ���� �� ����� �����
	int Rip_pk; // ���� �� ����� �����
	int Rip_dt; // �� �����
	int Rip_other; // ���� �� ��������� � ������ �����
	int Win_arena; //����� ������� �� �����
	int Rip_mob_this; // ���� �� ����� �� ���� �����
	int Rip_pk_this; // ���� �� ����� �� ���� �����
	int Rip_dt_this; // �� �� ���� �����
	int Rip_other_this; // ���� �� ��������� � ������ �� ���� �����
	long Exp_arena; //�������� ����� �� ���� �� �����
	long Exp_mob; //�������� �����  ���� �� ����� �����
	long Exp_pk; //�������� �����  ���� �� ����� �����
	long Exp_dt; //�������� �����  �� �����
	long Exp_other; //�������� �����  ���� �� ��������� � ������ �����
	long Exp_mob_this; //�������� �����  ���� �� ����� �� ���� �����
	long Exp_pk_this; //�������� �����  ���� �� ����� �� ���� �����
	long Exp_dt_this; //�������� �����  �� �� ���� �����
	long Exp_other_this; //�������� �����  ���� �� ��������� � ������ �� ���� �����
	/*����� ������ (�) �������� */
	//Polud ������ ����, ������� � ������� ����� ��������� �������-����������� � ������
	long ntfyExchangePrice;
	int HiredCost;// added by WorM (�������) 2010.06.04 ����� ����������� �� ����(������������ ��� �����)
};

struct player_special_data
{
	struct player_special_data_saved saved;

	char *poofin;		/* Description on arrival of a god. */
	char *poofout;		/* Description upon a god's exit.   */
	struct alias_data *aliases;	/* Character's aliases    */
	time_t may_rent;		/* PK control                       */
	int agressor;		/* Agression room(it is also a flag) */
	time_t agro_time;		/* Last agression time (it is also a flag) */
	struct _im_rskill_tag *rskill;	/* ��������� ������� */
	struct char_portal_type *portals;	/* ������� ������ ����� ��� */
	int *logs;		// ������ ����������� ������� log

	char *Exchange_filter;
	struct ignore_data *ignores;
	char *Karma; /* ������ � ����������, ���������� ���������*/

	struct logon_data * logons; /*������ � ������ ����*/

// Punishments structs
	struct punish_data pmute;
	struct punish_data pdumb;
	struct punish_data phell;
	struct punish_data pname;
	struct punish_data pfreeze;
	struct punish_data pgcurse;
	struct punish_data punreg;

	char *clanStatus; // ������ ��� ����������� �������� �� ���
	// TODO: ���������� ����������
	boost::shared_ptr<class Clan> clan; // ������ ����, ���� �� ����
	boost::shared_ptr<class ClanMember> clan_member; // ���� ������� � �����

	struct board_data *board; // ��������� ����������� ������� �� ������
};

class Player;
typedef boost::shared_ptr<Player> PlayerPtr;
typedef std::map < int/* ����� ������ */, int/* �������� ������ */ > CharSkillsType;
//typedef __gnu_cxx::hash_map < int/* ����� ������ */, int/* �������� ������ */ > CharSkillsType;

/**
* ����� ����� ��� �������/�����.
*/
class Character : public PlayerI
{
// �����
public:
	Character();
	virtual ~Character();

	// ��� ��� ��� ������ ��������... =)
	friend void save_char(CHAR_DATA *ch);
	friend void do_mtransform(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
	friend void medit_mobile_copy(CHAR_DATA * dst, CHAR_DATA * src);

	int get_skill(int skill_num);
	void set_skill(int skill_num, int percent);
	void clear_skills();
	int get_skills_count();

	int get_equipped_skill(int skill_num);
	int get_trained_skill(int skill_num);

	int get_obj_slot(int slot_num);
	void add_obj_slot(int slot_num, int count);

	////////////////////////////////////////////////////////////////////////////
	CHAR_DATA * get_touching() const;
	void set_touching(CHAR_DATA *vict);

	CHAR_DATA * get_protecting() const;
	void set_protecting(CHAR_DATA *vict);

	int get_extra_skill() const;
	CHAR_DATA * get_extra_victim() const;
	void set_extra_attack(int skill, CHAR_DATA *vict);

	CHAR_DATA * get_fighting() const;
	void set_fighting(CHAR_DATA *vict);

	// TODO: ����� ����� ������� � �������� (+ troom �� ������������, cast_spell/cast_subst/cast_obj ������ �� ����)
	void set_cast(int spellnum, int spell_subst, CHAR_DATA *tch, OBJ_DATA *tobj, ROOM_DATA *troom);
	int get_cast_spell() const;
	int get_cast_subst() const;
	CHAR_DATA * get_cast_char() const;
	OBJ_DATA * get_cast_obj() const;

	void clear_fighing_list();
	////////////////////////////////////////////////////////////////////////////

	int get_serial_num();
	void set_serial_num(int num);

	void purge(bool destructor = false);
	bool purged() const;

	const char * get_name() const;
	void set_name(const char *name);
	const char * get_pc_name() const;
	void set_pc_name(const char *name);
	const char * get_npc_name() const;
	void set_npc_name(const char *name);

	short get_class() const;
	void set_class(short chclass);

	short get_level() const;
	void set_level(short level);

	long get_idnum() const;
	void set_idnum(long idnum);

	int get_uid() const;
	void set_uid(int uid);

	long get_exp() const;
	void set_exp(long exp);

	short get_remort() const;
	void set_remort(short num);

	time_t get_last_logon() const;
	void set_last_logon(time_t num);

	////////////////////////////////////////////////////////////////////////////
	long get_gold() const;
	long get_bank() const;
	long get_total_gold() const;

	void add_gold(long gold, bool log = true);
	void add_bank(long gold, bool log = true);

	void set_gold(long num, bool log = true);
	void set_bank(long num, bool log = true);

	long remove_gold(long num, bool log = true);
	long remove_bank(long num, bool log = true);
	long remove_both_gold(long num, bool log = true);
	////////////////////////////////////////////////////////////////////////////

	int calc_morale() const;

	int get_con() const;
	void set_con(int);

	////////////////////////////////////////////////////////////////////////////
	void clear_add_affects();
	int get_con_add() const;
	void set_con_add(int);
	////////////////////////////////////////////////////////////////////////////

	/**
	 * \return ���� group �� ��� ����� ������� ����
	 * <= 1 - ������� ���� (����), >= 2 - ��������� ���� �� ��������� ���-�� �������
	 */
	int get_zone_group() const;

private:
	void check_fighting_list();
	void zero_init();

	CharSkillsType skills;  // ������ ��������� �������
	////////////////////////////////////////////////////////////////////////////
	CHAR_DATA *protecting_; // ���� ��� '��������'
	CHAR_DATA *touching_;   // ���� ��� '�����������'
	CHAR_DATA *fighting_;   // ���������
	bool in_fighting_list_;  // ������� ���� � ������ �������� �����������

	struct extra_attack_type extra_attack_; // ����� ���� ����, ����� � �.�.
	struct cast_attack_type cast_attack_;   // ���� ����������
	////////////////////////////////////////////////////////////////////////////
	int serial_num_; // ���������� ����� � ������ ����� (��� name_list)
	// true - ��� ������ � ���� ������ delete ��� ��������
	bool purged_;

/** TODO: ���� ���� �����������, ����� ���� private ���� */
	// ��� ���� ��� ������ ����
	std::string name_;
	// ��� ���� (��.�����)
	std::string short_descr_;
	// ��������� ����/����� ����
	short chclass_;
	// �������
	short level_;
	// id ���� (�� ���, ��� ��� ������), � ����� -1
	long idnum_;
	// uid (������ unique) ����
	int uid_;
	// �����
	long exp_;
	// �������
	short remorts_;
	// ����� ���������� ����� � ���� //by kilnik
	time_t last_logon_;
	// ������ �� �����
	long gold_;
	// ������ � �����
	long bank_gold_;
	// ������ ����
	int con_;
	// ����� �� ����
	int con_add_;

// ������
public:
	mob_rnum nr;		/* Mob's rnum                   */
	room_rnum in_room;	/* Location (real room number)   */
	int wait;			/* wait for how many loops         */
	int punctual_wait;		/* wait for how many loops (punctual style) */
	char *last_comm;		/* ��������� ������ ������� ����� ���������� ���� */

	struct char_player_data player_data;		/* Normal data                   */
	struct char_played_ability_data add_abils;		/* Abilities that add to main */
	struct char_ability_data real_abils;		/* Abilities without modifiers   */
	struct char_point_data points;		/* Points                       */
	struct char_special_data char_specials;		/* PC/NPC specials     */
	struct mob_special_data mob_specials;		/* NPC specials        */

	struct player_special_data *player_specials;	/* PC specials        */

	AFFECT_DATA *affected;	/* affected by what spells       */
	struct timed_type *timed;	/* use which timed skill/spells  */
	struct timed_type *timed_feat;	/* use which timed feats  */
	OBJ_DATA *equipment[NUM_WEARS];	/* Equipment array               */

	OBJ_DATA *carrying;	/* Head of list                  */
	DESCRIPTOR_DATA *desc;	/* NULL for mobiles              */

	long id;			/* used by DG triggers             */
	struct trig_proto_list *proto_script;	/* list of default triggers      */
	struct script_data *script;	/* script info for the object      */
	struct script_memory *memory;	/* for mob memory triggers         */

	CHAR_DATA *next_in_room;	/* For room->people - list         */
	CHAR_DATA *next;	/* For either monster or ppl-list  */
	CHAR_DATA *next_fighting;	/* For fighting list               */

	struct follow_type *followers;	/* List of chars followers       */
	CHAR_DATA *master;	/* Who is char following?        */

	struct spell_mem_queue MemQueue;		// ������� ��������� ����������

	int CasterLevel;
	int DamageLevel;
	struct PK_Memory_type *pk_list;
	struct helper_data_type *helpers;
	int track_dirs;
	int CheckAggressive;
	int ExtractTimer;

	FLAG_DATA Temporary;

	int Initiative;
	int BattleCounter;
	int round_counter;

	FLAG_DATA BattleAffects;

	int Poisoner;

	int *ing_list;		//����������� � ���� �����������
	load_list *dl_list;	// ����������� � ���� ��������
};

void change_fighting(CHAR_DATA * ch, int need_stop);
int fighting_list_size();

namespace CharacterSystem
{

void release_purged_list();

} // namespace CharacterSystem

#endif // CHAR_HPP_INCLUDED
