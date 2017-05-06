//
// Copyright (C) 2004-2006 Jasmine Langridge, ja-reiko@users.sourceforge.net
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//

struct joystick_s
{
    SDL_Joystick       *sdl_joystick;
    std::string         name;
    std::vector<float>  axis;
    std::vector<bool>   button;
    std::vector<vec2i>  hat;
};

class PApp
{
    public:

        enum StereoMode
        {
            StereoNone,
            StereoQuadBuffer,
            StereoRedBlue,
            StereoRedGreen,
            StereoRedCyan,
            StereoYellowBlue
        };

    private:

        std::string appname, apptitle;

        SDL_Window *screen; // TODO: rename this to "window" maybe
        SDL_GLContext context;
        
        /// Attempts to set fullscreen at native resolution.
        bool autoVideo = false;

    protected:

        int cx, cy, bpp;
        HiScore1 best_times;

    private:

        bool fullscr, noframe;
        bool reqRGB, reqAlpha, reqDepth, reqStencil;
        bool grabinput;

        StereoMode stereo;
        float stereoEyeTranslation;

        const uint8* sdl_keymap;
        int sdl_numkeys;
        uint8 sdl_mousemap;
        std::vector<joystick_s> sdl_joy;

        bool exit_requested, screenshot_requested;

        PSSRender *ssrdr;
        PSSTexture *sstex;
        PSSEffect *ssfx;
        PSSModel *ssmod;
        PSSAudio *ssaud;

    protected:

        // the derived app should keep these up to date
        vec3f cam_pos;
        mat44f cam_orimat;
        vec3f cam_linvel;

    public:

        PApp(const std::string &title = "PGame", const std::string &name = ".pgame"):
            appname(name), // for ~/.name
            apptitle(title), // for window title
            best_times("/players")
        {
            //PUtil::outLog() << "Initialising SDL" << std::endl;
            const int si = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_JOYSTICK);

            if (si < 0)
            {
                PUtil::outLog() << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
            }

            cx = cy = 0;
            bpp = 0;
            fullscr = false;
            noframe = false;
            exit_requested = false;
            screenshot_requested = false;
            reqRGB = reqAlpha = reqDepth = reqStencil = false;
            stereo = StereoNone;
            stereoEyeTranslation = 0.0f;
            grabinput = false;
        }

        virtual ~PApp()
        {
        }

        int run(int argc, char *argv[]);

    public:

        int getWidth() const
        {
            return cx;
        }

        int getHeight() const
        {
            return cy;
        }

        int getNumJoysticks()
        {
            return (sdl_joy.size());
        }

        int getJoyNumAxes(int j)
        {
            return (sdl_joy[j].axis.size());
        }

        int getJoyNumButtons(int j)
        {
            return (sdl_joy[j].button.size());
        }

        int getJoyNumHats(int j)
        {
            return (sdl_joy[j].hat.size());
        }

        float getJoyAxis(int j, int a)
        {
            return (sdl_joy[j].axis[a]);
        }

        bool getJoyButton(int j, int b)
        {
            return (sdl_joy[j].button[b]);
        }

        const vec2i &getJoyHat(int j, int h)
        {
            return (sdl_joy[j].hat[h]);
        }

        PSSRender & getSSRender()
        {
            return *ssrdr;
        }

        PSSTexture & getSSTexture()
        {
            return *sstex;
        }

        PSSEffect & getSSEffect()
        {
            return *ssfx;
        }

        PSSModel & getSSModel()
        {
            return *ssmod;
        }

        PSSAudio & getSSAudio()
        {
            return *ssaud;
        }

    protected:

        bool keyDown(int key)
        {
            return (sdl_keymap[key] != 0);
        }

        bool mouseButtonDown(int bt)
        {
            return ((sdl_mousemap & SDL_BUTTON(bt)) != 0);
        }

        void requestExit()
        {
            exit_requested = true;
        }

        void saveScreenshot()
        {
            screenshot_requested = true;
        }

        void grabMouse(bool grab = true);

        void drawModel(PModel &model);

        void stereoGLProject(float xmin, float xmax, float ymin, float ymax, float znear, float zfar, float zzps, float dist, float eye);
        void stereoFrustum(float xmin, float xmax, float ymin, float ymax, float znear, float zfar, float zzps, float eye);

        // config stuff

        void setScreenMode(int w, int h, bool fullScreen = false, bool hideFrame = false)
        {
            // use automatic video mode
            if (autoVideo)
            {
                SDL_DisplayMode dm;

                if (SDL_GetCurrentDisplayMode(0, &dm) == 0)
                {
                    cx = dm.w;
                    cy = dm.h;
                    fullscr = fullScreen;
                    noframe = hideFrame;
                    PUtil::outLog() << "Automatic video mode resolution: " << cx << 'x' << cy << std::endl;
                }
                else
                {
                    PUtil::outLog() << "SDL error, SDL_GetCurrentDisplayMode(): " << SDL_GetError() << std::endl;
                    autoVideo = false;
                }
            }

            // not written as an `else` branch because `autoVideo` may have
            // been updated in the case that automatic video mode failed
            if (!autoVideo)
            {
                cx = w;
                cy = h;
                fullscr = fullScreen;
                noframe = hideFrame;
            }
        }

        void setScreenBPP(int _bpp)
        {
            if (autoVideo)
                return;

            bpp = _bpp;
        }

        void setScreenModeAutoWindow();
        void setScreenModeFastFullScreen();

        void automaticVideoMode(bool av = false)
        {
            autoVideo = av;
        }

        void requireRGB(bool req = true)
        {
            reqRGB = req;
        }

        void requireAlpha(bool req = true)
        {
            reqAlpha = req;
        }

        void requireDepth(bool req = true)
        {
            reqDepth = req;
        }

        void requireStencil(bool req = true)
        {
            reqStencil = req;
        }

        void setStereoMode(StereoMode mode)
        {
            stereo = mode;
        }

        void setStereoEyeSeperation(float distance)
        {
            stereoEyeTranslation = distance * 0.5f;
        }

        // callbacks for derived classes

        virtual void config() /* throw (PUserException) */ ; // very light setup/config func
        virtual void load() /* throw (PUserException) */ ; // main resource loading
        virtual void unload(); // free resources

        virtual void tick(float delta);
        virtual void resize();
        virtual void render(float eyetranslation);
        virtual void keyEvent(const SDL_KeyboardEvent &ke);
        virtual void mouseButtonEvent(const SDL_MouseButtonEvent &mbe);
        virtual void mouseMoveEvent(int dx, int dy);
        virtual void cursorMoveEvent(int posx, int posy);
        virtual void joyButtonEvent(int which, int button, bool down);
};

