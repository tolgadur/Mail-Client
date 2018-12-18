#include "imap.hpp"
#include <iostream>

using namespace std;
using namespace IMAP;

// Message Class
IMAP::Message::Message(mailimap* imap, int uid, std::function<void()> updateUI): imap(imap), uid(uid), updateUI(updateUI){
  mailimap_set* set;
  mailimap_section* section;
  string msg_content;
  mailimap_fetch_type* fetch_type;
  mailimap_fetch_att* fetch_att;
  clist* fetch_result;
  int r;

  set = mailimap_set_new_single(uid);
  fetch_type = mailimap_fetch_type_new_fetch_att_list_empty();
  section = mailimap_section_new(nullptr);

  //Fetching Body
  fetch_att = mailimap_fetch_att_new_body_peek_section(section);
  mailimap_fetch_type_new_fetch_att_list_add(fetch_type, fetch_att);
  r = mailimap_uid_fetch(imap, set, fetch_type, &fetch_result);
  check_error(r, "could not fetch");
  getMsgBody(fetch_result);

  //Fetching Header
  fetch_att = mailimap_fetch_att_new_envelope();
  mailimap_fetch_type_new_fetch_att_list_add(fetch_type, fetch_att);
  r = mailimap_uid_fetch(imap, set, fetch_type, &fetch_result);
  check_error(r, "could not fetch");
  getMsgHeader(fetch_result);

  // mailimap_fetch_att_free(fetch_att);
  mailimap_fetch_type_free(fetch_type);
  mailimap_set_free(set);
  mailimap_section_free(section);
  mailimap_fetch_list_free(fetch_result);
}

string IMAP::Message::getBody(){
  return body;
}

string IMAP::Message::getField(string fieldname){
  if(fieldname == "Subject") return subject;
  else if(fieldname == "From") return from;
}

void IMAP::Message::deleteFromMailbox(){
  mailimap_set* set = mailimap_set_new_single(uid);
  mailimap_flag_list* flag_list = mailimap_flag_list_new_empty();
  mailimap_flag* flag = mailimap_flag_new_deleted();

  mailimap_flag_list_add(flag_list, flag);
  mailimap_store_att_flags* store = mailimap_store_att_flags_new_set_flags(flag_list);
  mailimap_uid_store(imap, set, store);
  mailimap_expunge(imap);

  //Update UI
  updateUI();

  // Releasing Memory
  mailimap_set_free(set);
  mailimap_flag_free(flag);
  mailimap_flag_list_free(flag_list);
  mailimap_store_att_flags_free(store);
}

void IMAP::Message::getMsgBody(clist* fetch_result){
  for(clistiter* iter = clist_begin(fetch_result); iter != nullptr; iter = clist_next(iter)){
    mailimap_msg_att* msg_att;
    string msg_body;
    string error = "error";

    msg_att = static_cast<mailimap_msg_att*>clist_content(iter);

    msg_body = getMsgBodyContent(msg_att);
    if(msg_body == error){
      continue;
    }
    body = msg_body;
  }
}

void IMAP::Message::getMsgHeader(clist* fetch_result){
  for(clistiter* iter = clist_begin(fetch_result); iter != nullptr; iter = clist_next(iter)){
    mailimap_msg_att* msg_att;
    string msg_subject;
    clist* frm_list;
    string error = "error";

    msg_att = static_cast<mailimap_msg_att*>clist_content(iter);
    msg_subject = getMsgSubjectContent(msg_att);
    if(msg_subject == error){
      continue;
    }
    subject = msg_subject;

    frm_list = getMsgFromContent(msg_att);
    if(frm_list == nullptr){
      continue;
    }
    from = castMsgFromToString(frm_list);
  }
}

string IMAP::Message::getMsgBodyContent(mailimap_msg_att* msg_att){
  for(clistiter* iter = clist_begin(msg_att->att_list) ; iter != nullptr ; iter = clist_next(iter)) {
    mailimap_msg_att_item* item;

    item = static_cast<mailimap_msg_att_item*>clist_content(iter);
    if (item->att_type != MAILIMAP_MSG_ATT_ITEM_STATIC) {
      continue;
    }

    if (item->att_data.att_static->att_type != MAILIMAP_MSG_ATT_BODY_SECTION) {
      continue;
    }

    return item->att_data.att_static->att_data.att_body_section->sec_body_part;
  }

  return "error";
}

string IMAP::Message::getMsgSubjectContent(mailimap_msg_att* msg_att){
  for(clistiter* iter = clist_begin(msg_att->att_list) ; iter != nullptr ; iter = clist_next(iter)) {
    mailimap_msg_att_item* item;

    item = static_cast<mailimap_msg_att_item*>clist_content(iter);
    if (item->att_type != MAILIMAP_MSG_ATT_ITEM_STATIC) {
      continue;
    }

    if (item->att_data.att_static->att_type != MAILIMAP_MSG_ATT_ENVELOPE) {
      continue;
    }

    return item->att_data.att_static->att_data.att_env->env_subject;
  }

  return "error";
}

clist* IMAP::Message::getMsgFromContent(mailimap_msg_att* msg_att){
  for(clistiter* iter = clist_begin(msg_att->att_list) ; iter != nullptr ; iter = clist_next(iter)) {
    mailimap_msg_att_item* item;

    item = static_cast<mailimap_msg_att_item*>clist_content(iter);
    if (item->att_type != MAILIMAP_MSG_ATT_ITEM_STATIC) {
      continue;
    }

    if (item->att_data.att_static->att_type != MAILIMAP_MSG_ATT_ENVELOPE) {
      continue;
    }

    return item->att_data.att_static->att_data.att_env->env_from->frm_list;  /* list of (mailimap_address *) */
  }

  return nullptr;
}

string IMAP::Message::castMsgFromToString(clist* frm_list){
  string msg_from;
  for(clistiter* iter = clist_begin(frm_list); iter != nullptr; iter = clist_next(iter)) {
    auto item = static_cast<mailimap_address*>clist_content(iter);
    if(item->ad_mailbox_name && item->ad_host_name){
      msg_from += item->ad_mailbox_name;
      msg_from += '@';
      msg_from += item->ad_host_name;
    }
    else{
      msg_from = "<UNKNOWN>";
    }
  }
  return msg_from;
}


// Session Class
IMAP::Session::Session(function<void()> updateUI): updateUI(updateUI), imap(mailimap_new(0,NULL)){}

Message** IMAP::Session::getMessages(){
  int numMails = getNumberMails();
  if(numMails == 0){
    // messages = new Message*[1];
    // messages[0] = nullptr;
    return messages;
  }

  if(messages){
    for(int i = 0; messages[i]; i++){
      //if(messages[i]){
        delete messages[i];
        messages[i] = nullptr;
      //}
    }
  }

  messages = new Message*[numMails + 1];
  mailimap_set* set;
  mailimap_fetch_type* fetch_type;
  mailimap_fetch_att* fetch_att;
  clist* fetch_result;
  int r;

  set = mailimap_set_new_interval(1, 0); // Fetch all messages
  fetch_type = mailimap_fetch_type_new_fetch_att_list_empty();
  fetch_att = mailimap_fetch_att_new_uid();
  mailimap_fetch_type_new_fetch_att_list_add(fetch_type, fetch_att);

  r = mailimap_fetch(imap, set, fetch_type, &fetch_result);
  check_error(r, "could not fetch");

  // For each message
  size_t count = 0;
	for(clistiter* iter = clist_begin(fetch_result); iter != nullptr; iter = clist_next(iter)){
		mailimap_msg_att* msg_att;
		int uid;
		msg_att = static_cast<mailimap_msg_att*>clist_content(iter);
		uid = getUID(msg_att);
		if(uid == 0) continue;

    messages[count] = new Message(imap, uid, updateUI);
    count++;
	}
  messages[count] = nullptr;

  //free
  //mailimap_fetch_att_free(fetch_att);
  mailimap_fetch_type_free(fetch_type);
  mailimap_set_free(set);
  return messages;
}

int IMAP::Session::getUID(mailimap_msg_att * msg_att){
  for(clistiter* iter = clist_begin(msg_att->att_list); iter != nullptr; iter = clist_next(iter)){
    mailimap_msg_att_item* item;

    item = static_cast<mailimap_msg_att_item*>clist_content(iter);
    if (item->att_type != MAILIMAP_MSG_ATT_ITEM_STATIC) {
      continue;
    }

    if (item->att_data.att_static->att_type != MAILIMAP_MSG_ATT_UID) {
      continue;
    }

    return item->att_data.att_static->att_data.att_uid;
  }

  return 0;
}

void IMAP::Session::connect(string const& server, size_t port){
  int r = mailimap_socket_connect(imap, server.c_str(), port);
  check_error(r, "could not connect");
}

void IMAP::Session::login(string const& userid, string const& password){
  int r = mailimap_login(imap, userid.c_str(), password.c_str());
  check_error(r, "could not login");
}

void IMAP::Session::selectMailbox(string const& m){
  mailbox = m;
  int r = mailimap_select(imap, mailbox.c_str());
  check_error(r, "could not select Mailbox");
}

int IMAP::Session::getNumberMails(){
  int numberMails = 0;
  mailimap_status_att_list* status_att_list = mailimap_status_att_list_new_empty();
  int r = mailimap_status_att_list_add(status_att_list, MAILIMAP_STATUS_ATT_MESSAGES);
  check_error(r, "status attribute could not be added");

  mailimap_mailbox_data_status* result;
  r = mailimap_status(imap, mailbox.c_str() , status_att_list, &result);
  check_error(r, "Information could not be returned");

  for(clistiter* iter = clist_begin(result->st_info_list); iter != nullptr; iter = clist_next(iter)){
    auto status_info = static_cast<mailimap_status_info*>clist_content(iter);

    if(status_info->st_att != MAILIMAP_STATUS_ATT_MESSAGES){
      continue;
    }
    numberMails = status_info->st_value;
  }

  //free memory
  mailimap_mailbox_data_status_free(result);
  mailimap_status_att_list_free(status_att_list);
  return numberMails;
}

IMAP::Session::~Session(){
    mailimap_logout(imap);
    mailimap_free(imap);
    for(int i = 0; messages[i]; i++)
      delete messages[i];
    delete [] messages;
}
