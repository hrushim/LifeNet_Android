package com.app.lifeNet;

import android.app.Activity;
import android.os.Bundle;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.TextView;

public class InboxView extends Activity {

	ListView msgListV;
	ArrayAdapter<String> adpList;

	@Override
	public void onCreate(Bundle savedInstanceState) {
		// Your code here
		super.onCreate(savedInstanceState);
		// setContentView(R.layout.inboxview);
		setContentView(R.layout.inboxview);

		msgListV = (ListView) findViewById(R.id.listView1);

		adpList = new ArrayAdapter<String>(this,
				R.layout.msglistitem);
		msgListV.setAdapter(adpList);

		if (MessageQueue.unreadCount == 0) {
			adpList.add("No messages");
		} else {
			for (int i = 0; i < MessageQueue.unreadCount; i++) {
				ChatMessage msg = (ChatMessage)MessageQueue.unreadMessageVector.elementAt(i);
				adpList.add("[" + msg.rxTime.toString() + "] " + msg.srcName + "\n" + msg.payload);
			}
		}
		
		MessageQueue.newMsgReadFlag = true;

	}

}
