package com.app.lifeNet;

import android.app.Activity;
import android.os.Bundle;
import android.view.KeyEvent;
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
			
			String[] strArr = LifeNet.selectedUserName.split(" ");

			if(LifeNet.SEQ_CNT == 0)
			{
				ChatMessage msg = new ChatMessage(LifeNetApi.getMyName(), 0, 4, "TEST", LifeNet.MSG_TYPE_DATA);
				UdpTx.send(LifeNetApi.getIpFromName(strArr[0]), msg, LifeNet.MSG_START_REP_CNT);
				LifeNet.SEQ_CNT++;
				
			}
			
			ChatMessage msg = new ChatMessage(LifeNetApi.getMyName(), LifeNet.SEQ_CNT, editText.getText().length(), editText.getText().toString(), LifeNet.MSG_TYPE_DATA);
			UdpTx.send(LifeNetApi.getIpFromName(strArr[0]), msg, LifeNet.MSG_REP_CNT);
			LifeNet.SEQ_CNT++;
			
		}
	};
	
}
