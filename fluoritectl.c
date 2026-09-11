#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define SOCKET_PATH "/tmp/fluorite.sock"

int main(int argc, char **argv)
{
	if (argc < 2)
	{
		fprintf(stderr, "Usage: fluoritectl <action> [arg]\n");
		return 1;
	}

	int sock = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sock < 0)
	{
		perror("socket");
		return 1;
	}

	struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("connect (is Fluorite running)");
        close(sock);
        return 1;
    }

	char command[256] = {0};
	for (int i = 1; i < argc; i++)
	{
		strcat(command, argv[i]);
		if (i < argc - 1) strcat(command, " ");
	}
	strcat(command, "\n");

	send(sock, command, strlen(command), 0);

	char res[32] = {0};
	ssize_t val = read(sock, res, sizeof(res) - 1);
	(void)val;
	printf("%s", res);

	close(sock);
	return 0;
}
