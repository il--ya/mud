/**
 * \file Contains definition of craft model for Bylins MUD.
 * \date 2015/12/28
 * \author Anton Gorev <kvirund@gmail.com>
 */

#ifndef __CRAFT_HPP__
#define __CRAFT_HPP__

#include "utils.h"
#include "obj.hpp"
#include "structs.h"
#include "sysdep.h"

#include <stdarg.h>

#include <set>
#include <map>
#include <list>
#include <array>
#include <string>

namespace pugi
{
	class xml_node;
}

namespace craft
{
	/**
	** \brief Loads Craft model.
	**/
	bool start();

	class CLogger
	{
	public:
		class CPrefix
		{
		public:
			CPrefix(CLogger& logger, const char* prefix) : m_logger(logger) { logger.push_prefix(prefix); }
			~CPrefix() { m_logger.pop_prefix(); }
			void change_prefix(const char* new_prefix) { m_logger.change_prefix(new_prefix); }

		private:
			CLogger& m_logger;
		};

		void operator()(const char* format, ...) __attribute__((format(printf, 2, 3)));

	private:
		void push_prefix(const char* prefix) { m_prefix.push_back(prefix); }
		void change_prefix(const char* new_prefix);
		void pop_prefix();

		std::list<std::string> m_prefix;
	};

	inline void CLogger::change_prefix(const char* new_prefix)
	{
		if (!m_prefix.empty())
		{
			m_prefix.back() = new_prefix;
		}
	}

	inline void CLogger::pop_prefix()
	{
		if (!m_prefix.empty())
		{
			m_prefix.pop_back();
		}
	}

	extern CLogger log;

	using id_t = std::string;	///< Common type for IDs.

	class CCases
	{
		const static int CASES_COUNT = CObjectPrototype::NUM_PADS;

		using cases_t = std::array<std::string, CASES_COUNT>;

	public:
		CCases() {}

		bool load_from_node(const pugi::xml_node* node);
		void load_from_object(const CObjectPrototype::shared_ptr& object);
		bool save_to_node(pugi::xml_node* node) const;

		const auto& get_case(const size_t number) const { return m_cases[number]; }
		const auto& aliases() const { return m_joined_aliases; }
		OBJ_DATA::pnames_t build_pnames() const;

	private:
		cases_t m_cases;
		std::list<std::string> m_aliases;
		std::string m_joined_aliases;

		friend class CMaterialClass;
		friend class CPrototype;
	};

	class CObject: public CObjectPrototype
	{
	public:
		CObject(const CObjectPrototype& object) : CObjectPrototype(object) {}
		CObject(const obj_vnum vnum) : CObjectPrototype(vnum) {}
		~CObject() {}

		bool load_from_node(const pugi::xml_node* node);
		void load_from_object(const CObjectPrototype::shared_ptr& object);

		bool save_to_node(pugi::xml_node* node) const;

		/**
		 * Builds OBJ_DATA instance suitable for add it into the list of objects prototypes.
		 *
		 * Allocates memory for OBJ_DATA instance and fill this memory by appropriate values.
		 *
		 * \return Pointer to created instance.
		 */
		OBJ_DATA* build_object() const;

	private:
		constexpr static int VALS_COUNT = 4;

		const static std::string KIND;

		using skills_t = std::map<ESkill, int>;
		using vals_t = std::array<int, VALS_COUNT>;
		using applies_t = std::list<obj_affected_type>;

		bool load_item_parameters(const pugi::xml_node* node);
		void load_skills(const pugi::xml_node* node);
		void load_extended_values(const pugi::xml_node* node);
		void load_applies(const pugi::xml_node* node);
		virtual bool check_object_consistency() const;

		const std::string& aliases() const { return m_cases.aliases(); }
		virtual const std::string& kind() const { return KIND; }

		CCases m_cases;

		friend class CCraftModel;
	};

	/**
	 * Describes one material class.
	 */
	class CMaterialClass
	{
	public:
		CMaterialClass(const std::string& id) : m_id(id) {}

		const auto& id() const { return m_id; }
		const auto& name() const { return m_item_cases.get_case(0); }

	private:
		bool load(const pugi::xml_node* node);
		bool load_adjectives(const pugi::xml_node* node);

		const std::string m_id;
		std::string m_short_desc;	///< Short description
		std::string m_long_desc;	///< Long description

		CCases m_item_cases;		///< Item cases (including aliases)
		CCases m_name_cases;		///< Name cases (including aliases)
		std::shared_ptr<CCases> m_male_adjectives;	///< Male adjective cases
		std::shared_ptr<CCases> m_female_adjectives;	///< Female adjective cases
		std::shared_ptr<CCases> m_neuter_adjectives;	///< Neuter adjective cases

		friend class CMaterial;
	};

	/**
	 * Defines material type.
	 */
	class CMaterial
	{
	public:
		CMaterial(const std::string& id) : m_id(id) {}

		const auto& id() const { return m_id; }
		const auto& classes() const { return m_classes; }

		const auto& get_name() const { return m_name; }
		void set_name(const std::string& _) { m_name = _; }

	private:
		bool load(const pugi::xml_node* node);

		const id_t m_id;						///< Material ID.
		std::string m_name;						///< Material name.
		std::list<CMaterialClass> m_classes;	///< List of material classes for this material.

		friend class CCraftModel;
	};

	class CMaterialInstance
	{
	public:

	private:
		const id_t m_type;
		const id_t m_class;
		int m_quality;
		int m_quantity;
	};

	/**
	 * Describes single recipe.
	 */
	class CRecipe
	{
	public:
		const auto& id() const { return m_id; }

	private:
		bool load(const pugi::xml_node* node);

		id_t m_id;                          ///< Recipe ID.
		std::string m_name;					///< Recipe name.

		friend class CCraftModel;
	};

	/**
	 * Describes crafting skill.
	 */
	class CSkillBase
	{
	public:
		CSkillBase() : m_threshold(0) {}

		const auto& id() const { return m_id; }

	private:
		bool load(const pugi::xml_node* node);

		id_t m_id;                          ///< Skill ID.
		std::string m_name;                 ///< Skill Name.
		int m_threshold;                    ///< Threshold to increase skill.

		friend class CCraftModel;
	};

	/**
	 * Describes single craft model.
	 */
	class CCraft
	{
	public:
		CCraft() : m_slots(0) {}

		const auto& id() const { return m_id; }

	private:
		bool load(const pugi::xml_node* node);

		id_t m_id;                              ///< Craft ID.
		std::string m_name;                     ///< Craft name.
		std::set<id_t> m_skills;                ///< List of required skills for this craft.
		std::set<id_t> m_recipes;               ///< List of available recipes for this craft.
		std::set<id_t> m_material;              ///< List of materials used by this craft.
		int m_slots;                            ///< Number of slots for additional skills.

		friend class CCraftModel;
	};

	/**
	 * Class to manage and store crafting model.
	 */
	class CCraftModel
	{
	public:
		using id_set_t = std::set<id_t>;
		using crafts_t = std::list<CCraft>;
		using skills_t = std::list<CSkillBase>;
		using recipes_t = std::list<CRecipe>;
		using materials_t = std::list<CMaterial>;
		using prototypes_t = std::list<CObjectPrototype::shared_ptr>;

		const static std::string FILE_NAME;
		constexpr static int DEFAULT_BASE_COUNT = 1;
		constexpr static int DEFAULT_REMORT_FOR_COUNT_BONUS = 1;
		constexpr static int DEFAULT_BASE_TOP = 75;
		constexpr static int DEFAULT_REMORTS_BONUS = 12;

		CCraftModel(): m_base_count(DEFAULT_BASE_COUNT),
			m_remort_for_count_bonus(DEFAULT_REMORT_FOR_COUNT_BONUS),
			m_base_top(DEFAULT_BASE_TOP),
			m_remorts_bonus(DEFAULT_REMORTS_BONUS)
		{
		}

		/**
		 * Loads data from XML data files.
		 *
		 * \return true if all data loaded successfully.
		 *         false otherwise.
		 */
		bool load();

		/**
		 * Merges loaded craft model into MUD world.
		 */
		bool merge();

		/**
		** For debug purposes. Should be removed when feature will be done.
		*/
		void create_item() const;

		const crafts_t& crafts() const { return m_crafts; }
		const skills_t& skills() const { return m_skills; }
		const recipes_t& recipes() const { return m_recipes; }
		const prototypes_t& prototypes() const { return m_prototypes; }
		const materials_t& materials() const { return m_materials; }

		const auto base_count() const { return m_base_count; }
		const auto remort_for_count_bonus() const { return m_remort_for_count_bonus; }
		const auto base_top() const { return m_base_top; }
		const auto remorts_bonus() const { return m_remorts_bonus; }

		bool export_object(const obj_vnum vnum, const  char* filename);

	private:
		/**
		** Describes VNums range.
		*/
		class CVNumRange
		{
		public:
			CVNumRange(const obj_vnum min, const obj_vnum max) : m_min(min), m_max(max) {}
			bool operator<(const CVNumRange& right) const { return m_min < right.m_min; }

			const obj_vnum& min() const { return m_min; }
			const obj_vnum& max() const { return m_max; }

		private:
			obj_vnum m_min;
			obj_vnum m_max;
		};

		enum EAddVNumResult
		{
			EAVNR_OK,
			EAVNR_OUT_OF_RANGE,
			EAVNR_ALREADY_EXISTS
		};

		/**
		** Loads N-th prototype from specified XML node and returns true
		** if it was successful and false otherwise.
		*/
		bool load_prototype(const pugi::xml_node* prototype, const size_t number);

		/**
		** Loads N-th material from specified XML node and returns true
		** if it was successful and false otherwise.
		*/
		bool load_material(const pugi::xml_node* material, const size_t number);

		bool load_from_node(const pugi::xml_node* node);

		bool load_vnum_ranges(const  pugi::xml_node* model);

		EAddVNumResult check_vnum(const obj_vnum vnum) const;
		EAddVNumResult add_vnum(const obj_vnum vnum);
		void report_vnum_error(const obj_vnum vnum, const EAddVNumResult add_vnum_result);

		crafts_t m_crafts;			///< List of crafts defined for the game.
		skills_t m_skills;			///< List of skills defined for the game.
		recipes_t m_recipes;     	///< List of recipes defined for the game.
		prototypes_t m_prototypes;	///< List of objects defined by the craft system.
		materials_t m_materials;	///< List of materials

		// Properties
		int m_base_count;						///< Base count of crafts (per character?).
		int m_remort_for_count_bonus;			///< Required remorts to take one more craft.
		int m_base_top;							///< Base top of skill level.
		int m_remorts_bonus;					///< Bonus to top skill level based on remorts count (how?)

		std::list<std::string> m_skill_files;		///< List of files with skill definitions.
		std::list<std::string> m_craft_files;		///< List of files with craft definitions.
		std::list<std::string> m_material_files;	///< List of files with material definitions.
		std::list<std::string> m_recipe_files;		///< List of files with recipe definitions.

		// Helpers
		std::map<id_t, id_set_t> m_skill2crafts;		///< Maps skill ID to set of crafts which this skill belongs to.
		std::map<id_t, const CCraft*> m_id2craft;		///< Maps craft ID to pointer to craft descriptor.
		std::map<id_t, const CSkillBase*> m_id2skill;	///< Maps skill ID to pointer to skill descriptor.
		std::map<id_t, const CRecipe*> m_id2recipe;		///< Maps recipe ID to pointer to recipe descriptor.

		std::set<CVNumRange> m_allowed_vnums;
		std::set<obj_vnum> m_existing_vnums;
	};
}

#endif // __CRAFT_HPP__

/* vim: set ts=4 sw=4 tw=0 noet syntax=cpp :*/
