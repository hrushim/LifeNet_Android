package com.app.lifeNet;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.os.Bundle;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Toast;


public class Settings extends Activity {

	public static String settingsFileLocation = "/data/data/com.app.lifeNet/settings.txt";
	Button saveButton;

	@Override
	public void onCreate(Bundle savedInstanceState) {
		// Your code here
		super.onCreate(savedInstanceState);
		setContentView(R.layout.lifenetsettings);

		saveButton = (Button) findViewById(R.id.button1);
		saveButton.setOnClickListener(saveClickListener);

		EditText userText = (EditText) findViewById(R.id.editText3);
		userText.setText(LifeNet.setUserName);
		EditText networknameText = (EditText) findViewById(R.id.editText1);
		networknameText.setText(LifeNet.setNetworkName);
		EditText wifiChannelText = (EditText) findViewById(R.id.editText2);
		wifiChannelText.setText(LifeNet.setWifiChannel);
		EditText ipText = (EditText) findViewById(R.id.editText4);
		ipText.setText(LifeNet.setIp);

		
	}

	private View.OnClickListener saveClickListener = new OnClickListener() {

		@Override
		public void onClick(View v) {
			// TODO Auto-generated method stub

			alertbox("Confirm Save", "Confirm Save Settings?");

		}
	};

	protected void alertbox(String title, String mymessage) {
		new AlertDialog.Builder(this)
				.setMessage(mymessage)
				.setTitle(title)
				.setPositiveButton("Yes",
						new DialogInterface.OnClickListener() {

							// do something when the button is clicked
							public void onClick(DialogInterface arg0, int arg1) {
								EditText userText = (EditText) findViewById(R.id.editText3);
								String userName = userText.getText().toString();
								EditText networknameText = (EditText) findViewById(R.id.editText1);
								String networkName = networknameText.getText()
										.toString();
								EditText wifiChannelText = (EditText) findViewById(R.id.editText2);
								String wifiChannel = wifiChannelText.getText()
										.toString();
								EditText ipText = (EditText) findViewById(R.id.editText4);
								String ip = ipText.getText().toString();

								StringBuffer sBuff = new StringBuffer();

								sBuff.append(userName + ";");
								sBuff.append(networkName + ";");
								sBuff.append(wifiChannel + ";");
								sBuff.append(ip + ";");

								File file = new File(settingsFileLocation);
								file.delete();
								try {
									FileOutputStream fOps = new FileOutputStream(
											settingsFileLocation);
									try {
										fOps.write(sBuff.toString().getBytes());
									} catch (IOException e) {
										// TODO Auto-generated catch block
										e.printStackTrace();
									}
									try {
										fOps.close();
									} catch (IOException e) {
										// TODO Auto-generated catch block
										e.printStackTrace();
									}

								} catch (FileNotFoundException e) {
									// TODO Auto-generated catch block
									e.printStackTrace();
								}

								Toast.makeText(getApplicationContext(), "Settings saved",
										Toast.LENGTH_SHORT).show();

							}
						})
				.setNegativeButton("No", new DialogInterface.OnClickListener() {

					// do something when the button is clicked
					public void onClick(DialogInterface arg0, int arg1) {

					}
				}).show();
	}
}
