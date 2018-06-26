#include "fight.extra.atack.hpp"

#include "char.hpp"
#include "spells.h"
#include "utils.h"

#include <boost/algorithm/string.hpp>

WeaponMagicalAtack::WeaponMagicalAtack(CHAR_DATA * ch) {ch_=ch;}

void WeaponMagicalAtack::set_atack(CHAR_DATA * ch, CHAR_DATA * victim) 
{
    OBJ_DATA *mag_cont;

    mag_cont = GET_EQ(ch, WEAR_QUIVER);
    if (GET_OBJ_VAL(mag_cont, 2) <= 0) 
    {
            act("�� ����� ������� ��� �� ����, � ��� ������ ����.", FALSE, ch, 0, 0, TO_CHAR);
            act("$n �������$g � ������� � ������$g ������ ����.", FALSE, ch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
            mag_cont->set_val(2,0);     
            return;
    }
    
    mag_single_target(GET_LEVEL(ch), ch, victim, NULL, GET_OBJ_VAL(mag_cont, 0), SAVING_REFLEX);
    
}
bool WeaponMagicalAtack::set_count_atack(CHAR_DATA * ch)
{
    OBJ_DATA *mag_cont;
    mag_cont = GET_EQ(ch, WEAR_QUIVER);
    //sprintf(buf, "���������� ��������� %d", get_count());
    //act(buf, TRUE, ch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
    //������� �� �������
    if ((GET_EQ(ch, WEAR_BOTHS)&&(GET_OBJ_TYPE(GET_EQ(ch, WEAR_BOTHS)) == OBJ_DATA::ITEM_WEAPON)) 
            && (GET_OBJ_SKILL(GET_EQ(ch, WEAR_BOTHS)) == SKILL_BOWS )
            && (GET_EQ(ch, WEAR_QUIVER)))
        {
            //���� � ��� � ����� ��� � ����� ������
            set_count(get_count()+1);
            
            //�� �������������� ����� 4 ������� ����������
            if (get_count() >= 4)
            {
                set_count(0);
                mag_cont->add_val(2,-1);
                return true;
                
            }
            else if ((can_use_feat(ch, DEFT_SHOOTER_FEAT))&&(get_count() == 3))
            {
                set_count(0);
                mag_cont->add_val(2,-1);
                return true;
            }
            return false;
        }
    set_count(0);
    return false;
}
