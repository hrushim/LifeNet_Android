package com.app.lifeNet;

import android.app.Activity;
import android.os.Bundle;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Toast;

public class MessageWindow extends Activity{

	@Override
	public void onCreate(Bundle savedInstanceState) {
		// Your code here
		super.onCreate(savedInstanceState);
		setContentView(R.layout.messagewindow);
		
		Button sendButton = (Button)findViewById(R.id.buttonSend);
		sendButton.setOnClickListener(sendOnClickListener);
		
	}
	
	public View.OnClickListener sendOnClickListener = new OnClickListener() {
		
		@Override
		public void onClick(View v) {
			// TODO Auto-generated method stub
			EditText editText = (EditText)findViewById(R.id.editTextSend);
			Toast.makeText(getApplicationContext(), editText.getText(), Toast.LENGTH_SHORT).show();
			
			ChatMessage msg = new ChatMessage(LifeNetApi.getMyName(), LifeNet.selectedUserName, LifeNet.seqNum, editText.getText().length(), editText.getText().toString(), 1);
			
			UdpTx.send("192.168.0.255", LifeNet.listenPort, msg);
		}
	};
	
}
