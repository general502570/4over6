package thunt.a4over6

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
        }
    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    external fun stringFromJNI(): String
    external fun stringclickedFromJNI(x: Int): String
    val JAVA_JNI_PIPE_JTOC_PATH = "/data/data/thunt.a4over6/4over6.jtoc"
    val JAVA_JNI_PIPE_CTOJ_PATH = "/data/data/thunt.a4over6/4over6.ctoj"

    fun ReadPipe(): String {
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

    companion object {

        // Used to load the 'native-lib' library on application startup.
        init {
            System.loadLibrary("native-lib")
        }
    }
}
