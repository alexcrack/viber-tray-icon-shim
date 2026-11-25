// LD_PRELOAD shim: replace Viber's colored tray pixmap with themed (monochrome) named icons.
// Calls Viber's bundled Qt 6.5 by mangled symbol name, so no Qt headers are needed.
#define _GNU_SOURCE 1
#include <dlfcn.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {
struct alignas(16) Obj { unsigned char buf[64]; };   // storage for QString/QIcon/QPixmap/QImage
struct QSize { int w, h; };

template <class F> F sym(const char *n) {
    void *p = dlvsym(RTLD_DEFAULT, n, "Qt_6");
    if (!p) p = dlsym(RTLD_DEFAULT, n);
    return reinterpret_cast<F>(p);
}

using setIcon_t   = void (*)(void *, const void *);
using fromUtf8_t  = void (*)(void *ret, long size, const char *data);
using fromTheme_t = void (*)(void *ret, const void *qstr);
using isNull_t    = bool (*)(const void *);
using pixmap_t    = void (*)(void *ret, const void *icon, const QSize *, int mode, int state);
using toImage_t   = void (*)(void *ret, const void *pix);
using dtor_t      = void (*)(void *);
using pixel_t     = unsigned (*)(const void *, int, int);
using dim_t       = int (*)(const void *);

bool debug = getenv("VIBER_TRAY_DEBUG") != nullptr;

bool makeThemeIcon(const char *name, Obj *out) {
    static auto fromUtf8  = sym<fromUtf8_t>("_ZN7QString8fromUtf8E14QByteArrayView");
    static auto fromTheme = sym<fromTheme_t>("_ZN5QIcon9fromThemeERK7QString");
    static auto isNull    = sym<isNull_t>("_ZNK5QIcon6isNullEv");
    if (!fromUtf8 || !fromTheme || !isNull) return false;
    Obj *s = new Obj;                       // intentionally leaked (created once)
    fromUtf8(s, (long)strlen(name), name);
    fromTheme(out, s);
    return !isNull(out);
}

// Count saturated red pixels (Viber's unread badge) in the original icon.
int redPixels(const void *icon) {
    static auto pixmap  = sym<pixmap_t>("_ZNK5QIcon6pixmapERK5QSizeNS_4ModeENS_5StateE");
    static auto toImage = sym<toImage_t>("_ZNK7QPixmap7toImageEv");
    static auto pixDtor = sym<dtor_t>("_ZN7QPixmapD1Ev");
    static auto imgDtor = sym<dtor_t>("_ZN6QImageD1Ev");
    static auto pixel   = sym<pixel_t>("_ZNK6QImage5pixelEii");
    static auto width   = sym<dim_t>("_ZNK6QImage5widthEv");
    static auto height  = sym<dim_t>("_ZNK6QImage6heightEv");
    if (!pixmap || !toImage || !pixDtor || !imgDtor || !pixel || !width || !height) return 0;
    Obj pm, img; QSize sz{32, 32};
    pixmap(&pm, icon, &sz, 0, 1 /*Off*/);
    toImage(&img, &pm);
    int n = 0, w = width(&img), h = height(&img);
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x) {
            unsigned c = pixel(&img, x, y);
            unsigned a = c >> 24, r = (c >> 16) & 255, g = (c >> 8) & 255, b = c & 255;
            if (a > 128 && r > 180 && g < 110 && b < 110) ++n;
        }
    imgDtor(&img); pixDtor(&pm);
    return n;
}
} // namespace

extern "C" void _ZN15QSystemTrayIcon7setIconERK5QIcon(void *self, const void *icon) {
    static auto real = reinterpret_cast<setIcon_t>(dlvsym(RTLD_NEXT, "_ZN15QSystemTrayIcon7setIconERK5QIcon", "Qt_6"));
    static Obj normal, unread;
    static bool haveNormal = makeThemeIcon(getenv("VIBER_TRAY_ICON") ?: "viber-tray-normal", &normal);
    static bool haveUnread = makeThemeIcon(getenv("VIBER_TRAY_ICON_UNREAD") ?: "viber-tray-unread", &unread);

    int red = redPixels(icon);
    if (debug) fprintf(stderr, "[viber-tray] setIcon: red=%d normal=%d unread=%d\n", red, haveNormal, haveUnread);
    if (red > 8 && haveUnread) return real(self, &unread);
    if (haveNormal) return real(self, &normal);
    real(self, icon);
}
