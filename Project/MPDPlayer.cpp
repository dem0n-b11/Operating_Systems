#include <iostream>
#include <libmpd-1.0/libmpd/libmpd.h>
#include <stdexcept>
#include "ConsoleColors.hpp"
#include "MPDPlayer.hpp"
#include <map>
#include <functional>

namespace MPD {
	static const size_t CONNECTION_TIMEOUT_VALUE = 10;
	
	/*Статический map для нахождения MPDPlayer объекта по 
	 * MpdObj, и функции для работы с ним.
	*/
	std::map<MpdObj *, MPDPlayer *>& Collection() {
		static std::map<MpdObj *, MPDPlayer *> collection;
		return collection;
	}
	
	void addToCollection(MpdObj *obj, MPDPlayer *player) {
		Collection().insert(std::make_pair(obj, player));
	}
	
	void deleteFromCollection(MpdObj *obj) {
		Collection().erase(obj);
	}
	
	/*Обработчик сигнала статуса.*/
	void status_changed(MpdObj *obj, ChangedStatusType status) {
		std::map<MpdObj *, MPDPlayer *> map = Collection();
		auto iter = map.find(obj);
		if(iter == map.end())
			throw std::runtime_error("Сannot find the MPDPlayer object.");
		(*iter).second->runUserSignalCallback(status);
	}
	
	/*Реализация класса Song.*/
	Song::Song(mpd_Song *s): _artist(s->artist), _title(s->title), _id(s->id), _pos(s->pos) {
	}
	const std::string& Song::artist() const {
		return _artist;
	}
	const std::string& Song::title() const {
		return _title;
	}
	size_t Song::id() const {
		return _id;
	}
	size_t Song::pos() const {
		return _pos;
	}
	
	/*Реализация класса SongList.*/
	void SongList::addSong(Song s) {
		songs.push_back(s);
	}
	SongList::const_iterator SongList::begin() const {
		return songs.begin();
	}
	SongList::const_iterator SongList::end() const {
		return songs.end();
	}
	
	/*Реализация класса MPDPlayer.*/
	MPDPlayer::MPDPlayer() {
		obj = NULL;
	}
	void MPDPlayer::connect(char *const host, char *const pass, int port) {
		obj = mpd_new(host, port, pass);
		if(obj == NULL)
			throw std::runtime_error("Сannot create object.");
		addToCollection(obj, this);
		if(mpd_set_connection_timeout(obj, CONNECTION_TIMEOUT_VALUE) != 0)
			throw std::runtime_error("Cannot set a connection timeout.");
		if(mpd_connect(obj) != MPD_OK)
			throw std::runtime_error("Cannot connect.");
		else
			mpd_send_password(obj);
	}
	void MPDPlayer::runUserSignalCallback(Status status) {
		function(*this, status);
	}
	void MPDPlayer::disconnect() {
		deleteFromCollection(obj);
		mpd_disconnect(obj);
		mpd_free(obj);
		obj = NULL;
	}
	void MPDPlayer::nextSong() {
		if(mpd_player_next(obj) != 0)
			throw std::runtime_error("Cannot change song.");
	}
	void MPDPlayer::prevSong() {
		if(mpd_player_prev(obj) != 0)
			throw std::runtime_error("Cannot change song.");
	}
	void MPDPlayer::play() {
		if(mpd_player_play(obj) != 0)
			throw std::runtime_error("Cannot play.");
	}
	void MPDPlayer::stop() {
		if(mpd_player_stop(obj) != 0)
			throw std::runtime_error("Cannot stop.");
	}
	void MPDPlayer::pause() {
		if(mpd_player_pause(obj) != 0)
			throw std::runtime_error("Cannot pause.");
	}
	void MPDPlayer::statusUpdate() {
		mpd_status_update(obj);
	}
	SongList MPDPlayer::getPlaylist() {
		MpdData *data = mpd_playlist_get_changes(obj,-1);
		SongList pl;
		if(data != NULL) {
			do {
				if(data->type == MPD_DATA_TYPE_SONG) {
					pl.addSong(Song(data->song));
				}
				data = mpd_data_get_next(data);
			} while(data != NULL);
		}
		return pl;
	}
	Song MPDPlayer::getCurrentSong() {
		mpd_Song *song = mpd_playlist_get_current_song(obj);
		if(song != NULL) {
			return Song(song);
		}
		throw std::runtime_error("Nothing is playing.");
	}
	void MPDPlayer::clearPlaylist() {
		if(mpd_playlist_clear(obj) != 0)
			throw std::runtime_error("Cannot clear a playlist.");
	}
	void MPDPlayer::deleteSong(size_t id) {
		if(mpd_playlist_delete_id(obj, id) != 0)
			throw std::runtime_error("Cannot delete a song from playlist.");
	}
	void MPDPlayer::playlistMove(size_t old_id, size_t new_id) {
		if(mpd_playlist_move_id(obj, old_id, new_id) != 0)
			throw std::runtime_error("Cannot move a song.");
	}
	void MPDPlayer::playlistShuffle() {
		if(mpd_playlist_shuffle(obj) != 0)
			throw std::runtime_error("Cannot shuffle.");
	}
	void MPDPlayer::playlistAdd(const std::string &path) {
		if(mpd_playlist_add(obj, path.c_str()) != 0)
			throw std::runtime_error("Cannot add to playlist.");
	}
	void MPDPlayer::playlistSave(const std::string &path) {
		if(mpd_database_save_playlist(obj, path.c_str()) != 0)
			throw std::runtime_error("Cannot save playlist.");
	}
	size_t MPDPlayer::getSongTotalTime() {
		return mpd_status_get_total_song_time(obj);
	}
	int MPDPlayer::getState() {
		return mpd_player_get_state(obj);
	}
	void MPDPlayer::setUserSignalCallback(std::function<void(MPDPlayer &, Status)> f) {
		if(obj == NULL)
			throw std::runtime_error("Player doesn't inicialized.");
		function = f;
		mpd_signal_connect_status_changed(obj,(StatusChangedCallback)status_changed, NULL);
	}
	std::vector<std::string> MPDPlayer::getPlaylistsList() {
		MpdData *data = mpd_database_playlist_list(obj);
		std::vector<std::string> pl;
		if(data != NULL) {
			do {
				if(data->type == MPD_DATA_TYPE_PLAYLIST) {
					pl.push_back(std::string(data->playlist->path));
				}
				data = mpd_data_get_next(data);
			} while(data != NULL);
		}
		return pl;
	}
	void MPDPlayer::openPlaylist(std::string &path) {
		MpdData *data = mpd_database_get_playlist_content(obj, path.c_str());
		if(data == NULL)
			throw std::runtime_error("Cannot open playlist.");
		this->clearPlaylist();
		do {
			if(data->type == MPD_DATA_TYPE_SONG) {
				this->playlistAdd(data->song->file);
			}
			data = mpd_data_get_next(data);
		} while(data != NULL);
	}
	std::vector<std::string> MPDPlayer::getAllSongs() {
		MpdData *data = mpd_database_get_complete(obj);
		if(data == NULL)
			throw std::runtime_error("Nothing found.");
		std::vector<std::string> pl;
		do {
			if(data->type == MPD_DATA_TYPE_SONG) {
				pl.push_back(data->song->file);
			}
			data = mpd_data_get_next(data);
		} while(data != NULL);
		return pl;
	}
	void MPDPlayer::updateDB() {
		if(mpd_database_update_dir(obj, "/") != 0)
			throw std::runtime_error("Cannot update the data base.");
	}
	void MPDPlayer::deletePlaylist(std::string &path) {
		if(mpd_database_delete_playlist(obj, path.c_str()) != 0)
			throw std::runtime_error("Cannot delete playlist.");
	}
	MPDPlayer::~MPDPlayer() {
		deleteFromCollection(obj);
		mpd_disconnect(obj);
		mpd_free(obj);
	}
}
