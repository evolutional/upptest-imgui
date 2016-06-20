# About #

µTest-imgui is a simple test runner ui for [µTest for C++11](https://github.com/evolutional/upptest). It is designed to be embedded alongside µTest in your application (for example, a game engine or tools framework).

We use the excellent [dear imgui](https://github.com/ocornut/imgui) for all ui functionality due to its lightweight and portable nature.


## Compiling ##

µTest-imgui has the following two external dependencies:

* [µTest for C++11](https://github.com/evolutional/upptest)
* [dear imgui](https://github.com/ocornut/imgui) by Omar Cornut

For simplicity, it's advised that both are added to your project as a `git submodule`.


# Usage #

Usage is simple. Include the header in your code:

	#include "upptest-imgui/upptest_imgui.h"


## Test Tree ##

On initialization of your application, create the test tree:

	auto test_tree = utest::imgui::create_tree(utest::registry::get().tests());


The test tree is a structure that organizes your tests into a tree hierarchy based on their names and categories. The hierarchy is created by splitting the category name by a delimiter (default is `'.'`).

For example, tests declared as follows:

	TEST(PlayerService_SpawnPlayer, "Services.PlayerService")
	{
	}

	TEST(PlayerService_UnspawnPlayer, "Services.PlayerService")
	{
	}

	TEST(PlayerService_KillPlayer, "Services.PlayerService")
	{
	}

	TEST(Messaging_SendMessage, "Services.Messaging")
	{
	}

	TEST(Messaging_HandleMessage, "Services.Messaging")
	{
	}

	TEST(GameObject_AddComponent, "GameObject")
	{
	}

	TEST(GameObject_GetComponent, "GameObject")
	{
	}

Will generate this tree:

	Services
		PlayerService
			PlayerService_SpawnPlayer
			PlayerService_UnspawnPlayer
			PlayerService_KillPlayer
		Messaging
			Messaging_SendMessage
			Messaging_HandleMessage
		GameObject
			GameObject_AddComponent
			GameObject_GetComponent


The tree can be built from iterator pairs too, should you need to initialize from a subset of tests or build multiple trees.

## Updating the UI ##

Once the tree has been created, you must tell `dear imgui` to update the UI.

To do this, you can call the following at any time between imgui's `NewFrame` and `Render` methods.

	utest::imgui::window(test_tree);

Calling `window` will create you a simple window with your tests in them.

If you wish to insert the test tree ui in your *own* window, you can use the following:

	utest::imgui::update_tree(test_tree);

Of course, if you have multiple trees of tests you must update them separately.


## License ##

µTest-imgui is free and unencumbered software released into the public domain. See UNLICENSE for details.

