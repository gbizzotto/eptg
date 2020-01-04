#include "mainwindow.h"

#include <initializer_list>
#include <signal.h>
#include <unistd.h>

#include <QApplication>

#include "eptg/project.hpp"

MainWindow * global_w;

void sig_handler(int)
{
	global_w->close();
	QCoreApplication::quit();
};

template<typename F>
void catchUnixSignals(std::initializer_list<int> quitSignals, F & handler)
{
	sigset_t blocking_mask;
	sigemptyset(&blocking_mask);
	for (auto sig : quitSignals)
		sigaddset(&blocking_mask, sig);

	struct sigaction sa;
	sa.sa_handler = handler;
	sa.sa_mask    = blocking_mask;
	sa.sa_flags   = 0;

	for (auto sig : quitSignals)
		sigaction(sig, &sa, nullptr);
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

	MainWindow w;
	global_w = &w;
	w.show();

	catchUnixSignals({SIGTERM, SIGINT}, sig_handler);

    return a.exec();
}
