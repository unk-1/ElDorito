#include <vector>
#include <deque>
#include <cstdint>

namespace ChatCommands
{
	int findNumberOfPlayersInGame();

	class AbstractChatCommand
	{
	public:

		virtual void doOnVotePass() = 0;
		virtual void doOnVoteStart() = 0;
		virtual bool isValidArgument(std::string s, std::string& returnInfo) = 0;
		virtual void doOnVoteFail() = 0;
		virtual bool isEnabled() = 0;
		void processMessage(uint8_t sender, std::string message);
		void StartVote();
		void enable();
		void disable();
		void tick();
		
		void endVoting();
		bool isCurrentlyVoting();
		std::string getName();
		std::string getDescription();
		AbstractChatCommand(std::string name, std::string description);
		void processVote(uint8_t sender, std::string message);

	protected:
		size_t votesNeeded = 0;

	private:
		std::string name;
		std::string description;
		bool currentlyVoting = false;
		time_t voteTimeStarted = 0;
		std::vector<uint64_t> yesVoters = std::vector<uint64_t>{};

	};


	class EndGameCommand : public AbstractChatCommand
	{
	public:
		virtual void doOnVoteStart();
		virtual void doOnVotePass();
		virtual void doOnVoteFail();
		virtual bool isValidArgument(std::string s, std::string& returnInfo);
		virtual bool isEnabled();
		EndGameCommand();

	};
	class ShuffleTeamsCommand : public AbstractChatCommand
	{
	public:
		virtual void doOnVoteStart();
		virtual void doOnVotePass();
		virtual void doOnVoteFail();
		virtual bool isValidArgument(std::string s, std::string& returnInfo);
		virtual bool isEnabled();
		ShuffleTeamsCommand();

	};

	class KickPlayerCommand : public AbstractChatCommand
	{
	public:
		virtual void doOnVoteStart();
		virtual void doOnVotePass();
		virtual void doOnVoteFail();
		virtual bool isEnabled();
		virtual bool isValidArgument(std::string s, std::string& returnInfo);
		KickPlayerCommand();

	private:
		std::string playerName;
	};

	class KickIndexCommand : public AbstractChatCommand
	{
	public:
		virtual void doOnVoteStart();
		virtual void doOnVotePass();
		virtual void doOnVoteFail();
		virtual bool isEnabled();
		virtual bool isValidArgument(std::string s, std::string& returnInfo);
		KickIndexCommand();

	private:
		std::string playerName;
	};


}