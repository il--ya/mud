#ifndef __ACT_WIZARD_HPP__
#define __ACT_WIZARD_HPP__

#include "sysdep.h"

#include <memory>
#include <map>

struct inspect_request
{
	int sfor;			//��� �������
	int unique;			//UID
	int fullsearch;			//������ ����� ��� ���
	int found;			//������� ����� �������
	char *req;			//���������� ��� ������
	char *mail;			//����
	int pos;			//������� � �������
	struct logon_data * ip_log;	//���� ������ �� ������� ���� �����
	struct timeval start;		//����� ����� ��������� ������ ��� �������
	std::string out;		//����� � ������� ������������� �����
	bool sendmail; // ���������� �� �� ���� ������ �����
};

using InspReqPtr = std::shared_ptr<inspect_request>;
using InspReqListType = std::map<int /* filepos, ������� � player_table ����� ������� ������ ������ */, InspReqPtr /* ��� ������ */>;

extern InspReqListType& inspect_list;

struct setall_inspect_request
{
	int unique; // UID
	int found; //������� �������
	int type_req; // ��� �������: ����, ����� ����, ������ ��� �� (0, 1, 2 ��� 3)
	int freeze_time; // �����, �� ������� ������
	char *mail; // ���� ������
	char *pwd; // ������
	char *newmail; // ����� ����
	char *reason; // ������� ��� �����
	int pos; // ������� � �������
	struct timeval start;		//����� ����� ��������� ������ ��� �������
	std::string out;		//����� � ������� ������������� �����
};

using SetAllInspReqPtr = std::shared_ptr<setall_inspect_request>;
using SetAllInspReqListType = std::map<int, SetAllInspReqPtr>;

extern SetAllInspReqListType& setall_inspect_list;

void inspecting();
void setall_inspect();

#endif // __ACT_WIZARD_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
