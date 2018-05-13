package thunt.a4over6

import android.app.Activity
import android.content.Intent
import android.net.VpnService
import android.support.v7.app.AppCompatActivity
import android.os.Bundle
import kotlinx.android.synthetic.main.activity_main.*
import java.io.*

class MainActivity : AppCompatActivity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        // Example of a call to a native method
        sample_text.text = stringFromJNI()

        mainbutton.setOnClickListener{
            sample_text.text = stringclickedFromJNI(1)
            threadCPP()
            sample_text.text = beginWork()
        }
    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    external fun stringFromJNI(): String
    external fun stringclickedFromJNI(x: Int): String
    external fun startCPP(): Int
    val JAVA_JNI_PIPE_JTOC_PATH = "/data/data/thunt.a4over6/4over6.jtoc"
    val JAVA_JNI_PIPE_CTOJ_PATH = "/data/data/thunt.a4over6/4over6.ctoj"
    lateinit var ipPacket: IpPacket

    class IpPacket(info: String) {
        val infopiece = info.split(" ")
        val ip = infopiece[0]
        val route = infopiece[1]
        val dns1 = infopiece[2]
        val dns2 = infopiece[3]
        val dns3 = infopiece[4]
    }

    fun ReadPipe(): String? {
        val file = File(JAVA_JNI_PIPE_CTOJ_PATH)
        val bufferedReader: BufferedReader = file.bufferedReader()
        val inputString = bufferedReader.use { it.readLine() }
        return inputString
    }

    fun WritePipe(cont: String) {
        val file = File(JAVA_JNI_PIPE_JTOC_PATH)
        val bufferWriter: BufferedWriter = file.bufferedWriter()
        bufferWriter.use { out->out.write(cont) }
    }

    fun StartVPN() {
        val intent = VpnService.prepare(this)
        if (intent != null) {
            startActivityForResult(intent, 0);
        } else {
            onActivityResult(0, Activity.RESULT_OK, null)
        }
    }

    fun threadCPP() : Thread {
        val thread = object: Thread(){
            public override fun run() {
                startCPP()
            }
        }
        thread.start()
        return thread
    }

    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        if (resultCode == Activity.RESULT_OK){
            val serviceIntent = Intent(this, MyVpnService::class.java)
            serviceIntent.putExtra("ip", ipPacket.ip)
            serviceIntent.putExtra("route", ipPacket.route)
            serviceIntent.putExtra("dns1", ipPacket.dns1)
            serviceIntent.putExtra("dns2", ipPacket.dns2)
            serviceIntent.putExtra("dns3", ipPacket.dns3)
        }
        super.onActivityResult(requestCode, resultCode, data)
    }

    fun beginWork() : String {
        var ipstring = ReadPipe()
        while (ipstring == null) {
            ipstring = ReadPipe()
        }
        return "Successfully read!"
    }

    companion object {

        // Used to load the 'native-lib' library on application startup.
        init {
            System.loadLibrary("native-lib")
        }
    }
}
