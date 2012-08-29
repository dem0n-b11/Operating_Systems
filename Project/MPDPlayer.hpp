#pragma once

#include <libmpd-1.0/libmpd/libmpd.h>
#include <string>
#include <iostream>
#include <functional>
#include <vector>
#include <stdexcept>

namespace MPD {
	class Song {
	private:
		std::string _artist;
		std::string _title;
		size_t _id;
		size_t _pos;
	public:
		explicit Song(mpd_Song *s);
		const std::string& artist() const;
		const std::string& title() const;
		size_t id() const;
		size_t pos() const;
	};
	
	class SongList {
	private:
		std::vector<Song> songs;
		void addSong(Song s);
	public:
		typedef std::vector<Song>::const_iterator const_iterator;
		friend class MPDPlayer;
		
		const_iterator begin() const;
		const_iterator end() const;
	};
	
	typedef ChangedStatusType Status;
		
	class MPDPlayer {
	private:
		MpdObj *obj;
		std::function<void(MPDPlayer &, ChangedStatusType)> function;
		void runUserSignalCallback(Status status);
	public:
		MPDPlayer();
		friend void status_changed(MpdObj *obj, Status status);
		
		void connect(char *const host, char *const pass, int port);
		void disconnect();
		void nextSong();
		void prevSong();
		void play();
		void play(size_t id);
		void stop();
		void pause();
		void statusUpdate();
		SongList getPlaylist();
		Song getCurrentSong();
		size_t getSongTotalTime();
		int getState();
		void clearPlaylist();
		void deleteSong(size_t id);
		void playlistMove(size_t old_id, size_t new_id);
		void playlistShuffle();
		void playlistAdd(const std::string &path);
		void playlistSave(const std::string &path);
		void setUserSignalCallback(std::function<void(MPDPlayer &, Status status)>);
		std::vector<std::string> getPlaylistsList();
		void openPlaylist(std::string &path);
		std::vector<std::string> getAllSongs();
		void updateDB();
		void deletePlaylist(std::string &path);
		
		~MPDPlayer();
	};
}
