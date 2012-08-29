#include <iostream>
#include "MPDPlayer.hpp"
#include <string>
#include "ConsoleColors.hpp"
#include <algorithm>

using MPD::MPDPlayer;
using MPD::SongList;
using MPD::Song;

int main(int argc, char *argv[]) {
	MPDPlayer player;
	if(argc != 3) {
		char host[] = "localhost";
		char pass[] = "";
		size_t port = 6600;
		try {
			player.connect(host, pass, port);
		} catch(std::exception &ex) {
			std::cout << RED << "ERROR: " << YELLOW << ex.what() << CLEAR << "\n";
			exit(EXIT_FAILURE);
		}
	} else {
		try {
			player.connect(argv[1], argv[2], 6600);
		} catch(std::exception &ex) {
			std::cerr << RED << "ERROR: " << YELLOW << ex.what() << CLEAR << "\n";
			exit(EXIT_FAILURE);
		}
	}
	try {
		player.setUserSignalCallback([](MPDPlayer &pl, MPD::Status status) {
			if(status & MPD_CST_SONGID) {
				try{
					auto song = pl.getCurrentSong();
					std::string tmp = "";
					size_t sec = pl.getSongTotalTime()%60;
					if(sec < 10)
						tmp = "0";
					std::cout << GREEN << "Now playing: " << BLUE << song.artist() << CLEAR << " - " << YELLOW << song.title() << CLEAR << "\n";
					std::cout << GREEN << "Total song time: " << BLUE << 
					pl.getSongTotalTime()/60 << ":" << tmp << sec
						<< "\n" << CLEAR;
				} catch(...) {}
			}
			if(status & MPD_CST_STATE) {
				std::cout << GREEN << "State: " << CLEAR;
				switch(pl.getState()) {
					case MPD_PLAYER_PLAY:
						std::cout << BLUE << "Playing\n" << CLEAR;
						break;
					case MPD_PLAYER_PAUSE:
						std::cout << BLUE << "Paused\n" << CLEAR;
						break;
					case MPD_PLAYER_STOP:
						std::cout << BLUE << "Stopped\n" << CLEAR;
						break;
					default:
						break;
				}
			}
		});
	} catch(std::exception &ex) {
		std::cout << RED << "ERROR: " << YELLOW << ex.what() << CLEAR << "\n";
		exit(EXIT_FAILURE);
	}
	std::string command;
	std::cout << GREEN << "*** Hello" << YELLOW << " :)" << GREEN << " ***\n" << CLEAR;
	player.statusUpdate();
	while(true) {
		try {
			std::cout << GREEN << "Command: " << CLEAR;
			std::cin >> command;
			if(command == "play") {
				player.play();
			} else if(command == "pause") {
				player.pause();
			} else if(command == "next") {
				player.nextSong();
			} else if(command == "prev") {
				player.prevSong();
			} else if(command == "exit") {
				player.stop();
				player.disconnect();
				std::cout << GREEN << "*** Goodbye " << YELLOW << ":)" << GREEN << " ***\n" << CLEAR;
				exit(EXIT_SUCCESS);
			} else if(command == "stop") {
				player.stop();
			} else if(command == "list") {
				std::cout << GREEN << "Playlist:\n" << CLEAR;
				auto pl = player.getPlaylist();
				std::for_each(pl.begin(), pl.end(), [](const Song &s) {
					std::cout << s.id() << ". " << BLUE << s.artist() <<
						CLEAR << " - " << s.title() << "\n";
				});
			} else if(command == "now") {
				try{
					auto s = player.getCurrentSong();
					std::cout << GREEN << "Now playing: " << CLEAR;
					std::cout << BLUE << s.artist() <<
						CLEAR << " - " << YELLOW << s.title() << CLEAR << "\n";
				} catch(std::exception &e) {
					std::cout << RED << "WARNING: " << YELLOW << e.what() << CLEAR << "\n";
				}
			} else if(command == "save") {
				std::cout << GREEN << "Enter the name of the new playlist: " << CLEAR;
				std::string path;
				std::cin >> path;
				player.playlistSave(path.c_str());
			} else if(command == "playlists") {
				auto pl = player.getPlaylistsList();
				std::cout << GREEN << "Saved playlists:\n" << CLEAR;
				std::for_each(pl.begin(), pl.end(), [](std::string &s) {
					std::cout << BLUE << s << CLEAR << "\n";
				});
			} else if(command == "open") {
				std::cout << GREEN << "Enter the name of the playlist: " << CLEAR;
				std::string path;
				std::cin >> path;
				player.openPlaylist(path);
			} else if(command == "clear") {
				std::cout << GREEN << "Clear: " << YELLOW << "completed.\n" << CLEAR;
				player.clearPlaylist();
			} else if(command == "update") {
				std::cout << GREEN << "Update: \n" << YELLOW << "complete.\n" << CLEAR;
				player.updateDB();
			} else if(command == "all") {
				auto pl = player.getAllSongs();
				std::cout << GREEN << "Songs found:" << CLEAR << "\n";
				std::for_each(pl.begin(), pl.end(), [](std::string &s) {
					std::cout << BLUE << s << CLEAR << "\n";
				});
			} else if(command == "add") {
				std::cout << GREEN << "Enter the name of file: " << CLEAR;
				std::string path;
				std::cin >> path;
				player.playlistAdd(path);
			} else if(command == "shuffle") {
				std::cout << GREEN << "Shuffle: " << YELLOW << "complete.\n" << CLEAR;
				player.playlistShuffle();
			} else if(command == "play_next") {
				std::cout << GREEN << "Enter the id of the next song: " << CLEAR;
				size_t id;
				std::cin >> id;
				player.playlistMove(id, player.getCurrentSong().pos() + 1);
			} else if(command == "delete_playlist") {
				std::cout << GREEN << "Enter the name of playlist: " << CLEAR;
				std::string path;
				std::cin >> path;
				player.deletePlaylist(path);
			} else if(command == "delete_song") {
				std::cout << GREEN << "Enter the id of the song: " << CLEAR;
				size_t id;
				std::cin >> id;
				player.deleteSong(id);
			} else {
				std::cout << RED << "WARNING: " << YELLOW << "incorrect command.\n" << CLEAR;
			}
			player.statusUpdate();
		} catch(std::exception &ex) {
			std::cerr << RED << "ERROR: " << YELLOW << ex.what() << CLEAR << "\n";
			exit(EXIT_FAILURE);
		}
	}
	return 0;
}
