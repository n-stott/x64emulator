#ifndef IOCTL_H
#define IOCTL_H

namespace kernel {

    enum class Ioctl {
        fioclex,
        fionclex,
        fionbio,
        tcgets,
        tcsets,
        tcsetsw,
        tiocgwinsz,
        tiocswinsz,
        tiocgpgrp,
    };
}

#endif