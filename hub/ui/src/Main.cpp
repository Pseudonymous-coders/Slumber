#include <MACE/MACE.h>
#define ASSETS_FOLDER "/home/liavt/Desktop/Slumber/hub/ui/assets/"

using namespace mc;

gfx::ProgressBar bar;
gfx::Group group;

gfx::ogl::Texture selection;
gfx::ogl::Texture background;
gfx::ogl::Texture foreground;

void create(){
    gfx::Renderer::setRefreshColor(Colors::DARK_GRAY);

    selection = gfx::ogl::Texture(ASSETS_FOLDER+std::string("progressBar.png"));
    background = gfx::ogl::Texture();
    foreground = gfx::ogl::Texture();

    gfx::ogl::checkGLError(__LINE__, __FILE__);

    selection.setParameter(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    selection.setParameter(GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    background.init();
    float backgroundData[] = { 0,0,0,0 };

    background.setData(backgroundData, 1, 1, GL_FLOAT, GL_RGBA);

    background.setParameter(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    background.setParameter(GL_TEXTURE_MIN_FILTER, GL_NEAREST);


    foreground.init();
    float foregroundData[] = { 0,1,0,1 };

    foreground.setData(foregroundData, 1, 1, GL_FLOAT, GL_RGBA);

    foreground.setParameter(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    foreground.setParameter(GL_TEXTURE_MIN_FILTER, GL_NEAREST);


    bar = gfx::ProgressBar(0, 100, 50);
    bar.setHeight(0.25f);
    bar.setWidth(0.5f);
    bar.setSelectionTexture(selection);
    bar.setBackgroundTexture(background);
    bar.setForegroundTexture(foreground);

    group.addChild(bar);
}

int main(int argc, char** argv){
    os::WindowModule window = os::WindowModule(720,720,"Slumber Hub");
    window.setFPS(30);
    window.setFullscreen(false);
    window.setCreationCallback(&create);
    MACE::addModule(window);

    window.addChild(group);

    MACE::init();
    while(MACE::isRunning()){
        MACE::update();

        std::this_thread::sleep_for(std::chrono::milliseconds(33));
    }
    MACE::destroy();

    return 0;
}
