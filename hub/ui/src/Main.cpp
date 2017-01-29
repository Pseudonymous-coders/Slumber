#include <MACE/MACE.h>
#include <SlumberUI.h>
#define ASSETS_FOLDER "D:/Workspace/Slumber/hub/ui/assets/"

using namespace mc;

gfx::ProgressBar bar;
gfx::Text barProgress, statusMessage;
gfx::Group group;

gfx::ogl::Texture selection;
gfx::ogl::Texture background;
gfx::ogl::Texture foreground;

void setProgress(const unsigned int prog) {
	bar.easeTo(static_cast<float>(prog), 60.0f);
	barProgress.setText(std::to_wstring(prog));
}

void setStatus(const std::wstring& message) {
	statusMessage.setText(message);
}

void create(){
    gfx::Renderer::setRefreshColor(Colors::DARK_GRAY);

    selection = gfx::ogl::Texture(ASSETS_FOLDER+std::string("progressBar.png"));
    background = gfx::ogl::Texture(ASSETS_FOLDER + std::string("background.png"));
    foreground = gfx::ogl::Texture(ASSETS_FOLDER + std::string("foreground.png"));

    bar = gfx::ProgressBar(0, 100, 0);
    bar.setHeight(0.25f);
    bar.setWidth(0.5f);
    bar.setSelectionTexture(selection);
    bar.setBackgroundTexture(background);
    bar.setForegroundTexture(foreground);

    group.addChild(bar);

	gfx::Font f = gfx::Font::loadFont(ASSETS_FOLDER + std::string("consola.ttf"));
	f.setSize(86);

	barProgress = gfx::Text("0", f);
	barProgress.setY(0.3f);

	f.setSize(48);
	statusMessage = gfx::Text("", f);
	statusMessage.setY(0.75f);

	group.addChild(statusMessage);

	group.addChild(barProgress);

	setProgress(70);
	setStatus(L"Hi");
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
