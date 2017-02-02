#define MACE_DEBUG 1

//MACE INCLUDES
#include <MACE/MACE.h>

//SLUMBER INCLUDES
#include <slumberui.h>
#include <util/log.hpp>

//STANDARD INCLUDES
#include <iostream>

//BOOST INCLUDES
#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/chrono.hpp>

using namespace mc;

gfx::ProgressBar bar;
gfx::Text barProgress, statusMessage, temperature, humidity, movement;
gfx::Image droplet, thermometer, vibrations, power;
gfx::Group group;

namespace slumber {
		volatile bool isCreated = false;
}

void slumber::setProgress(const unsigned int prog) {
	bar.easeTo(static_cast<float>(prog), 60.0f);
	barProgress.setText(std::to_wstring(prog));
}

void slumber::setStatus(const std::wstring& message) {
	statusMessage.setText(message);
}

void slumber::setHumidity(const int humid) {
	humidity.setText(std::to_wstring(humid) + L" RH");
}

void slumber::setTemperature(const int temp) {
	temperature.setText(std::to_wstring(temp) + L"\u00B0");
}

void slumber::setMovement(const int move) {
	movement.setText(std::to_wstring(move) + L"%");
}

void create() {
	std::cout << "\n\n\n\nCREATING UI\n\n\n\n\n" << std::endl;
	gfx::Renderer::setRefreshColor(Colors::DARK_GRAY);

	std::cout << ASSETS_FOLDER << "progressBar.png" << std::endl;

	gfx::ColorAttachment selection(ASSETS_FOLDER + std::string("progressBar.png"));

	bar = gfx::ProgressBar(0, 100, 0);
	bar.setHeight(0.25f);
	bar.setWidth(0.5f);
	bar.setX(-0.4f);
	bar.setSelectionTexture(selection);
	bar.setBackgroundTexture(ASSETS_FOLDER + std::string("background.png"));
	bar.setForegroundTexture(gfx::ColorAttachment(selection, Colors::LIGHT_GREEN));

	group.addChild(bar);

	gfx::Font f = gfx::Font::loadFont(ASSETS_FOLDER + std::string("consola.ttf"));
	f.setSize(86);

	barProgress = gfx::Text("0", f);
	barProgress.setTexture(Colors::LIGHT_GREEN);
	bar.addChild(barProgress);

	f.setSize(48);
	statusMessage = gfx::Text("", f);
	statusMessage.setY(-0.75f);
	statusMessage.setTexture(Colors::WHITE);

	group.addChild(statusMessage);

	droplet = gfx::Image(ASSETS_FOLDER + std::string("droplet.png"));
	droplet.setWidth(0.15f);
	droplet.setHeight(0.15f);
	droplet.setY(0.5f);
	droplet.setX(0.5f);
	group.addChild(droplet);

	humidity = gfx::Text("", f);
	humidity.setTexture(Colors::WHITE);
	humidity.setX(0.3f);
	droplet.addChild(humidity);

	thermometer = gfx::Image(ASSETS_FOLDER + std::string("thermometer.png"));
	thermometer.setWidth(0.15f);
	thermometer.setHeight(0.15f);
	thermometer.setX(0.5f);
	group.addChild(thermometer);

	temperature = gfx::Text("", f);
	temperature.setTexture(Colors::WHITE);
	temperature.setX(0.3f);
	thermometer.addChild(temperature);

	vibrations = gfx::Image(ASSETS_FOLDER + std::string("vibrations.png"));
	vibrations.setWidth(0.15f);
	vibrations.setHeight(0.15f);
	vibrations.setX(0.5f);
	vibrations.setY(-0.5f);
	group.addChild(vibrations);

	movement = gfx::Text("", f);
	movement.setTexture(Colors::WHITE);
	movement.setX(0.3f);
	vibrations.addChild(movement);

	power = gfx::Image(ASSETS_FOLDER + std::string("power.png"));
	power.setWidth(0.1f);
	power.setHeight(0.1f);
	power.setX(0.9f);
	power.setY(0.9f);
	//group.addChild(power);

	slumber::setProgress(50);
}

void slumber::__loop_run() {
	os::WindowModule window = os::WindowModule(720, 720, "Slumber Hub");
	window.setFPS(30);
	//window.setFullscreen(true);
	window.setCreationCallback(&create);
	MACE::addModule(window);

	window.addChild(group);

	MACE::init();

	while(MACE::isRunning()) {
		MACE::update();

		/*
		yo copernicus - over here

		put all the functions you want run in the main loop here
		you can change the updates per second down there
		it wont affect the fps of rendering, as long as MACE::update() is called reasonably

		also use these functions to change the ui. they must be called in between MACE::init() and MACE::destroy()
		
		slumber::setProgress(70);
		slumber::setHumidity(50);
		slumber::setStatus(L"Hi");
		slumber::setTemperature(42);
		slumber::setMovement(50);
		*/

		boost::this_thread::sleep_for(boost::chrono::milliseconds(33));
	}
	MACE::destroy();

}

void slumber::runUI() {
	boost::thread ui_thread(boost::bind(slumber::__loop_run));
	while(!MACE::isRunning());
	Logger::Log("GUIINTE", SW("STARTED GUI INTERFACE")); 
}
