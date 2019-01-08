#ifndef IMAP_H
#define IMAP_H
#include "imaputils.hpp"
#include <libetpan/libetpan.h>
#include <string>
#include <functional>
 

namespace IMAP {

class Message {
private:
	std::string body;
	std::string subject;
	std::string from;
	mailimap* imap;
	int uid;
	std::function<void()> updateUI;
public:
	Message(mailimap* imap, int uid, std::function<void()> updateUI);
	/**
	 * Get the body of the message. You may chose to either include the headers or not.
	 */
	std::string getBody();
	/**
	 * Get one of the descriptor fields (subject, from, ...)
	 */
	std::string getField(std::string fieldname);


	void deleteFromMailbox();

	// Methods needed for fetching
	void getMsgBody(clist* fetch_result);
	void getMsgHeader(clist* fetch_result);
	std::string getMsgBodyContent(mailimap_msg_att* msg_att);
	std::string getMsgSubjectContent(mailimap_msg_att* msg_att);
	clist* getMsgFromContent(mailimap_msg_att* msg_att);
	std::string castMsgFromToString(clist* from_list);
};

class Session {
private:
	mailimap* imap;
	std::string mailbox;
	Message** messages = nullptr;
	std::function<void()> updateUI;
public:
	Session(std::function<void()> updateUI);

	/**
	 * Get all messages in the INBOX mailbox terminated by a nullptr (like we did in class)
	 */
	Message** getMessages();

	/**
	 * connect to the specified server (143 is the standard unencrypte imap port)
	 */
	void connect(std::string const& server, size_t port = 143);

	/**
	 * log in to the server (connect first, then log in)
	 */
	void login(std::string const& userid, std::string const& password);

	/**
	 * select a mailbox (only one can be selected at any given time)
	 *
	 * this can only be performed after login
	 */
	void selectMailbox(std::string const& m);

	// Method to get the uid of a mailimap_msg_att
	int getUID(mailimap_msg_att * msg_att);

	// Method to get the number of mails in the mailbox
	int getNumberMails();

	~Session();


};
}

#endif /* IMAP_H */
