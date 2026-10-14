#pragma once
#include <memory>
namespace halo2 {
class DiscordLogin;
class OnlineDirectory;
class OnlineAccount;
class OnlineMatchmaking;
// Desktop service ownership can cross the frontend/game transition without
// copying credentials into launch data or narrowing native pointers.
struct NativeTu15OnlineServices {
    std::shared_ptr<DiscordLogin> login;
    std::shared_ptr<OnlineDirectory> directory;
    std::shared_ptr<OnlineAccount> account;
    std::shared_ptr<OnlineMatchmaking> matchmaking;
};
}
