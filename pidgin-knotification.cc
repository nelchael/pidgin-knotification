#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusConnectionInterface>

#include <purple.h>
#include <status.h>

PurplePlugin *myself = NULL;

static void Notify(PurpleBuddy *buddy, const gchar *what) {
	const char *name = NULL;
	if (purple_buddy_get_contact_alias(buddy))
		name = purple_buddy_get_contact_alias(buddy);
	else if (purple_buddy_get_alias(buddy))
		name = purple_buddy_get_alias(buddy);
	else if (purple_buddy_get_server_alias(buddy))
		name = purple_buddy_get_server_alias(buddy);
	else
		name = purple_buddy_get_name(buddy);

#if 0
	purple_debug_info("KDE notification", "name=\"%s\" what=\"%s\"\n", name, what);
#endif

	QString q_who = QString::fromUtf8(name);
	QString q_what = QString::fromUtf8(what);

	if (q_what.length() > 75) {
		q_what.truncate(74);
		q_what += QString("...");
	}

	const QString dbusServiceName = "org.freedesktop.Notifications";
	const QString dbusInterfaceName = "org.freedesktop.Notifications";
	const QString dbusPath = "/org/freedesktop/Notifications";

	QDBusConnectionInterface* interface = QDBusConnection::sessionBus().interface();
	if (!interface || !interface->isServiceRegistered(dbusServiceName)) {
		purple_debug_error("KDE notification", "D-Bus service not registered\n");
		return;
	}

	QList<QVariant> args;
	args.append("pidgin");
	args.append(0U);
	args.append("dialog-information");
	args.append("Pidgin");
	args.append(q_who + QString(": ") + q_what);
	args.append(QStringList());
	args.append(QVariantMap());
	args.append(5 * 1000);			// TODO: Make this configurable

	QDBusMessage m = QDBusMessage::createMethodCall(dbusServiceName, dbusPath, dbusInterfaceName, "Notify");
	m.setArguments(args);

	QDBusMessage replyMsg = QDBusConnection::sessionBus().call(m);
	if (replyMsg.type() == QDBusMessage::ErrorMessage)
		purple_debug_error("KDE notification", "D-Bus call failed: %s\n", replyMsg.errorMessage().toStdString().c_str());
}

extern "C" {

	static void callback_new_message(PurpleAccount *account, const gchar *sender, const gchar *message, int, gpointer) {
		PurpleBuddy *buddy = purple_find_buddy(account, sender);
		PurpleConversation *conv = purple_find_conversation_with_account(PURPLE_CONV_TYPE_IM, purple_buddy_get_name(buddy), account);
		if (purple_conversation_has_focus(conv))
			return;
		PurpleStatusPrimitive psp = purple_primitive_get_type_from_id(purple_status_get_id(purple_account_get_active_status(account)));
#if 0
		purple_debug_info("KDE notification", "status = %d\n", psp);
#endif
		if (psp == PURPLE_STATUS_UNSET || psp == PURPLE_STATUS_OFFLINE || psp == PURPLE_STATUS_UNAVAILABLE || psp == PURPLE_STATUS_INVISIBLE || psp == PURPLE_STATUS_EXTENDED_AWAY)
			return;
		gchar *body = purple_markup_strip_html(message);
		Notify(buddy, body);
		g_free(body);
	}

	static void callback_new_chat(PurpleAccount *account, const gchar *sender, const gchar *message, PurpleConversation*, gpointer) {
		PurpleBuddy *buddy = purple_find_buddy(account, sender);
		gchar *body = purple_markup_strip_html(message);
		Notify(buddy, body);
		g_free(body);
	}

	static gboolean plugin_load(PurplePlugin *plugin) {
		void *conv_handle = purple_conversations_get_handle();

		purple_signal_connect(conv_handle, "received-im-msg", plugin, PURPLE_CALLBACK(callback_new_message), NULL);
		purple_signal_connect(conv_handle, "received-chat-msg", plugin, PURPLE_CALLBACK(callback_new_chat), NULL);

		return TRUE;
	}

	static gboolean plugin_unload(PurplePlugin *plugin) {
		void *conv_handle = purple_conversations_get_handle();

		purple_signal_disconnect(conv_handle, "received-im-msg", plugin, PURPLE_CALLBACK(callback_new_message));
		purple_signal_disconnect(conv_handle, "received-chat-msg", plugin, PURPLE_CALLBACK(callback_new_chat));

		return TRUE;
	}

	static PurplePluginInfo info = {
		PURPLE_PLUGIN_MAGIC,
		PURPLE_MAJOR_VERSION,
		PURPLE_MINOR_VERSION,
		PURPLE_PLUGIN_STANDARD,
		0,
		0,
		NULL,
		PURPLE_PRIORITY_DEFAULT,

		(char*)"core-nelchael-pidgin-knotification",
		(char*)"KDE notification",
		(char*)"0.3",
		(char*)"Use KDE notifications for new messages",
		(char*)"Use KDE notifications for new messages",

		(char*)"Krzysztof Pawlik <krzysiek.pawlik@people.pl>",
		(char*)"http://bitbucket.org/nelchael/pidgin-knotification",

		plugin_load,
		plugin_unload,
		NULL,
		NULL,
		NULL,
		NULL,

		NULL,
		NULL,
		NULL,
		NULL,
		NULL
	};

	static void init_plugin(PurplePlugin *plugin) {
		myself = plugin;
	}

	PURPLE_INIT_PLUGIN(pidgin_knotification, init_plugin, info);

}
