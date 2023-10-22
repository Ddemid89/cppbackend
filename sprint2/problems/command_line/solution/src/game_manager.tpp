namespace game_manager {

template <class Handler>
void GameSession::AddPlayer(PlayerInfo info, Handler&& handler) {
    net::dispatch(strand_,
        [this, info = std::move(info), handler = std::forward<Handler>(handler)]
        {
            move_manager::State state;

            if (random_spawn_) {
                state = move_map_.GetRandomPlace();
            } else {
                state = move_map_.GetStartPlace();
            }

            players_.emplace_back(info.Id, std::move(info.name), state);
            id_for_player_[info.Id] = &players_.back();
            handler(info);
        }
    );
}

template<class Handler>
void GameSession::GetPlayers(Handler&& handler) const {
    net::dispatch(
        strand_,
        [this, handler = std::forward<Handler>(handler)](){
            handler(players_, Result::ok);
        }
    );
}

template<class Handler>
void GameSession::MovePlayer(PlayerId player_id, move_manager::Direction dir, Handler&& handler) {
    net::dispatch(
        strand_,
        [this, player_id, dir, handler = std::forward<Handler>(handler)] {
            auto it = id_for_player_.find(player_id);

            if (it != id_for_player_.end()) {
                Player& player = *it->second;
                if (dir != move_manager::Direction::NONE) {
                    player.state.dir = dir;
                }
                player.state.speed = GetSpeed(dir);
                handler(Result::ok);
            } else {
                throw std::runtime_error("GameSession: player not found");
            }
        }
    );
}







template<class Handler>
void GameManager::Join(std::string name, model::Map::Id map, Handler&& handler) {
    PlayerInfo p_info;
    p_info.name = std::move(name);
    p_info.Id = GetUniquePlayerId();
    AddPlayer(std::move(p_info), std::move(map), std::forward<Handler>(handler));
}

template<class Handler>
void GameManager::AddPlayer(PlayerInfo p_info, model::Map::Id map, Handler&& handler) {
    net::dispatch(
        tokens_strand_,
        [this, p_info = std::move(p_info), map = std::move(map), handler = std::forward<Handler>(handler)]()mutable{
            Token token = GetUniqueToken();
            while (tokens_.find(token) != tokens_.end()) {
                token = GetUniqueToken();
            }
            p_info.token = *token;
            tokens_[token] = p_info.Id;
            FindOrCeateSession(std::move(p_info), std::move(map), std::forward<Handler>(handler));
        }
    );
}

template<class Handler>
void GameManager::FindOrCeateSession(PlayerInfo p_info, model::Map::Id map, Handler&& handler) {
    net::dispatch(
        sessions_strand_,
        [this, p_info = std::move(p_info), map = std::move(map), handler = std::forward<Handler>(handler)]()mutable{
            auto it = sessions_for_maps_.find(map);
            GameSession* session = nullptr;

            if (it == sessions_for_maps_.end()) {
                sessions_.emplace_back(ioc_, *FindMap(map), *maps_index_.at(map), random_spawn_);
                sessions_for_maps_[map].push_back(&sessions_.back());
                session = &sessions_.back();
            } else {
                for (GameSession* sess : it->second) {
                    if (sess->BookPlace()) {
                        session = sess;
                        break;
                    }
                }
                if (session == nullptr) {
                    sessions_.emplace_back(ioc_, *FindMap(map), *maps_index_.at(map), random_spawn_);
                    sessions_for_maps_[map].push_back(&sessions_.back());
                    session = &sessions_.back();
                }
            }

            players_for_sessions_[p_info.Id]   = session;
            session->AddPlayer(std::move(p_info), std::forward<Handler>(handler));
        }
    );
}



template<class Handler>
void GameManager::GetPlayers(Token token, Handler&& handler) {
    FindSession(token,
        [handler = std::forward<Handler>(handler)]
        (std::optional<GameSession*> session, PlayerId id, Result res)mutable{
            if (res == Result::ok) {
                GameSession& sess = **session;
                sess.GetPlayers(std::forward<Handler>(handler));
            } else {
                handler(std::nullopt, res);
            }
        }
    );
}

template<class Handler>
void GameManager::MovePlayer(Token token, move_manager::Direction dir, Handler&& handler) {
    FindSession(std::move(token),
        [dir, handler = std::forward<Handler>(handler)]
        (std::optional<GameSession*> session, PlayerId id, Result res)mutable{
            if (res == Result::ok) {
                GameSession& sess = **session;
                sess.MovePlayer(id, dir, handler);
            } else {
                handler(res);
            }
        }
    );
}

template<class Handler>
void GameManager::CallTick(uint64_t duration, Handler&& handler) {
    if (test_mode_) {
        Tick(duration, std::forward<Handler>(handler));
    } else {
        throw std::logic_error("Game not in test mode");
    }
}

template<class Handler>
void GameManager::Tick(u_int64_t duration, Handler&& handler) {
    //LOG(duration);
    net::dispatch(
        sessions_strand_,
        [this, duration, handler = std::forward<Handler>(handler)] (){
            for (GameSession& session : sessions_) {
                session.Tick(duration);
            }
            handler(Result::ok);
        }
    );
}


template<class Handler>
void GameManager::FindToken(Token token, Handler&& handler) {
    net::dispatch(
        tokens_strand_,
        [this, token = std::move(token), handler = std::forward<Handler>(handler)]()mutable{
            auto it = tokens_.find(token);
            if (it != tokens_.end()) {
                handler(it->second);
            } else {
                handler(std::nullopt);
            }
        }
    );
}

template<class Handler>
void GameManager::FindSession(Token token, Handler&& handler) {
    FindToken(std::move(token),
        [this, handler = std::forward<Handler>(handler)](std::optional<PlayerId> id)mutable{
            if (id.has_value()) {
                net::dispatch(sessions_strand_,
                    [this, id, handler = std::forward<Handler>(handler)]()mutable{
                        auto it = players_for_sessions_.find(*id);
                        if (it != players_for_sessions_.end()) {
                            handler(it->second, *id, Result::ok);
                        } else {
                            handler(std::nullopt, 0, Result::no_session);
                        }
                    }
                );
            } else {
                handler(std::nullopt, 0, Result::no_token);
            }
        }
    );
}

} // namespace game_manager
