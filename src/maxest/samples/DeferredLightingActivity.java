package maxest.samples;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.Reader;
import java.io.StringWriter;
import java.io.Writer;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import android.app.Activity;
import android.content.res.AssetManager;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.view.Window;
import android.view.WindowManager;



public class DeferredLightingActivity extends Activity
{
	public static native void passInputData(String gbufferVertexShader, String gbufferPixelShader, String lightVertexShader, String lightPixelShader, String materialVertexShader, String materialPixelShader, String meshGeometryData, String unitSphereGeometryData);
	public static native void rendererOnSurfaceChanged(int width, int height);
	public static native void rendererOnDrawFrame();
	
    
    
    static
    {
        System.loadLibrary("deferred_lighting");
    }
    
    
    
    public String convertStreamToString(InputStream is) throws IOException
    {
        if (is != null)
        {
            Writer writer = new StringWriter();
            char[] buffer = new char[8*1024];
            
            try
            {
                Reader reader = new BufferedReader(new InputStreamReader(is));
                int n;
                
                while ((n = reader.read(buffer)) != -1)
                {
                    writer.write(buffer, 0, n);
                }
            }
            finally
            {
                is.close();
            }
            
            return writer.toString();
        }
        else
        {       
            return "";
        }
    }
    
    
    
    GLSurfaceView glSurfaceView;
    static String gbufferVertexShader = "", gbufferPixelShader = "", lightVertexShader = "", lightPixelShader = "", materialVertexShader = "", materialPixelShader = "", unitSphereGeometryData = "", meshGeometryData = "";
	

    
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        
        AssetManager assetManager = getAssets();

        try
        {
        	InputStream is1 = assetManager.open("gbuffer.vs");
        	gbufferVertexShader = convertStreamToString(is1);
        	InputStream is2 = assetManager.open("gbuffer.ps");
        	gbufferPixelShader = convertStreamToString(is2);
        	InputStream is3 = assetManager.open("light.vs");
        	lightVertexShader = convertStreamToString(is3);
        	InputStream is4 = assetManager.open("light.ps");
        	lightPixelShader = convertStreamToString(is4);        	
        	InputStream is5 = assetManager.open("material.vs");
        	materialVertexShader = convertStreamToString(is5);
        	InputStream is6 = assetManager.open("material.ps");
        	materialPixelShader = convertStreamToString(is6);
        	InputStream is7 = assetManager.open("unit_sphere_geometry.mp3");
        	unitSphereGeometryData = convertStreamToString(is7);                	
        	InputStream is8 = assetManager.open("mesh_geometry.mp3");
        	meshGeometryData = convertStreamToString(is8);	
        }
        catch (Exception ex)
        {
        	
        }

        passInputData(gbufferVertexShader, gbufferPixelShader, lightVertexShader, lightPixelShader, materialVertexShader, materialPixelShader, unitSphereGeometryData, meshGeometryData);

        this.requestWindowFeature(Window.FEATURE_NO_TITLE);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN);

        glSurfaceView = new GLSurfaceView(this);
        glSurfaceView.setEGLContextClientVersion(2);
        glSurfaceView.setEGLConfigChooser(5, 6, 5, 0, 16, 8);
        glSurfaceView.setRenderer(new Renderer());
        glSurfaceView.setRenderMode(GLSurfaceView.RENDERMODE_CONTINUOUSLY);
        setContentView(glSurfaceView);        
    }

    
    
    @Override protected void onPause()
    {
        super.onPause();
        glSurfaceView.onPause();
    }

    
    
    @Override protected void onResume()
    {
        super.onResume();
        glSurfaceView.onResume();
    }
    
    
    
    private static class Renderer implements GLSurfaceView.Renderer
    {
    	static long timeCounter = 0;
    	static long lastTime = 0;
    	static long FPS = 0;
    	
        public void onSurfaceCreated(GL10 gl, EGLConfig config)
        {
        	lastTime = System.currentTimeMillis();
        }
        
        public void onSurfaceChanged(GL10 gl, int width, int height)
        {
        	DeferredLightingActivity.rendererOnSurfaceChanged(width, height);     	
        }
    	
        public void onDrawFrame(GL10 gl)
        {
        	DeferredLightingActivity.rendererOnDrawFrame();

            long currentTime = System.currentTimeMillis();
            long deltaTime = currentTime - lastTime;
            lastTime = currentTime;

            timeCounter += deltaTime;
            FPS++;
            
            if (timeCounter >= 1000)
            {
            	System.out.println(FPS + "    " + (float)timeCounter/(float)FPS);
            	
            	timeCounter -= 1000;
            	FPS = 0;
            }
        }
    }
}
