connect:
mp_masterserver - config var

struct Client
{
	Adr;
	bool is_admin, ok;
	float timer;
}
c -> ID_HELLO

Calling Connect and not calling SetMaximumIncomingConnections acts as a dedicated client.
GetLastPing
GetLowestPing
SetSplitMessageProgressInterval
hardcore server


SERVER
in config:
max_connections = 32 ?
secret = "pswd"

Po��czenie: dodaje client, ok = false, ma 5 sekund na wys�anie ID_HELLO potem jest od��czany


--------------------------------------------------------------------------------
KOMENDY
ID_HELLO - wysy�ane przez klienta z wersj� gry i protoko�u
w przysz�o�ci b�dzie mo�na doda� login i has�o
serwer odsy�a ID_HELLO
0 - ok
1 - jest nowa wersja (+dword wersja) - to ma wy�szy priorytet ni� wersja protoko�u
2 - z�a wersja protoko�u
3 - broken
Serwer ustawia ok = true

ID_HOST - klient za�o�y� serwer (nazwa, max graczy, flagi)

ID_HOST_STOP

ID_LIST - klient chce pobiera� list� serwer�w

ID_LIST_STOP - klient chce przesta� pobiera� list� serwer�w

ID_JOIN

ID_LOGIN loguje jako admin, ustawia is_admin = true
klient -> secret
server -> ID_LOGIN 0 lub 1 (0 - nieprawid�owy)

ID_CMD
	restart
	shutdown
	