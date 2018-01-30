//���� ������� ����� ������ ������� ������������
#define MAX_VOLT_LEN	512
//���� ������� ���� ��� ����� ������
#define VOLT_METR_FILE	"../voltmetr.log.gz"

typedef struct  {
	int	pulse;
	int type;
	unsigned long long start;
	unsigned long long stop;
} _voltMetr;


int voltMetrFlush();
_voltMetr *voltMetrStart(int type,int pulse);
int voltMetrStop(_voltMetr *p);
//���� ���� ����� ������ � ������ (����� ���� ���������)