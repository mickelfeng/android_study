package com.hook;


import static de.robv.android.xposed.XposedHelpers.findAndHookMethod;

import android.app.Application;
import android.content.Context;
import android.util.Log;

import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.Arrays;

import de.robv.android.xposed.IXposedHookLoadPackage;
import de.robv.android.xposed.XC_MethodHook;
import de.robv.android.xposed.XposedBridge;
import de.robv.android.xposed.XposedHelpers;
import de.robv.android.xposed.callbacks.XC_LoadPackage;

public class hook implements IXposedHookLoadPackage {
    //定义包名
    private static final String TAG = "XposedModule";
    private static XC_LoadPackage.LoadPackageParam lpparam = null;

    private static Object new_obj = null;

    @Override
    public void handleLoadPackage(XC_LoadPackage.LoadPackageParam _lpparam) throws ClassNotFoundException {
        lpparam = _lpparam;
        // 过滤不必要的应用
//        if (!lpparam.packageName.equals(PACKAGE_NAME)) return;
        // 执行Hook
        hook(lpparam);
    }

    private void hook(XC_LoadPackage.LoadPackageParam lpparam) throws ClassNotFoundException {
        Log.d(TAG, "handleLoadPackage: " + lpparam.packageName);

        Class<?> clazz = XposedHelpers.findClass("android.net.Activity", lpparam.classLoader);
//        Class<?> classs = lpparam.classLoader.loadClass("dalvik.system.DexFile");;
//        hook_one_method(classs,"attach", Context.class);
//        hook_all_method(classs,"defineClass");

        hook_all_method(clazz);
    }

    private void hook_one_method(Class<?> clazz, String methodName, Object... parameterTypesAndCallback) {
        findAndHookMethod(Application.class, "attach",
                Context.class, new XC_MethodHook() {
                    @Override
                    protected void beforeHookedMethod(MethodHookParam param) throws Throwable {
                        super.beforeHookedMethod(param);
                        if (param == null) {
                            return;
                        }
                        //输出所有参数
                        for (Object o : param.args) {
                            Log.d(TAG, "beforeHookedMethod参数：" + o);
                        }
                    }

                    @Override
                    protected void afterHookedMethod(MethodHookParam param) throws Throwable {
                        super.afterHookedMethod(param);
                        Log.d(TAG, "afterHookedMethod返回值：" + param.getResult());
                        if (param == null) {
                            return;
                        }
                        //输出所有参数
                        for (Object o : param.args) {
                            Log.d(TAG, "afterHookedMethod参数：" + o);
                        }

                        ClassLoader cl = ((Context) param.args[0]).getClassLoader();
                        Class<?> hookclass = null;
                        try {
                            hookclass = cl.loadClass("com.appsflyer.internal.AFa1nSDK$30218$AFa1wSDK$25697");
                            Log.d(TAG, "load success");
                        } catch (Exception e) {
                            Log.d(TAG, "load class error");
                            return;
                        }
                    }
                });
    }

    private void hook_all_method(Class<?> clazz) {
        String className = clazz.getName();
        Method[] m = clazz.getDeclaredMethods();

        //hook构造函数
        XposedBridge.hookAllConstructors(clazz, new XC_MethodHook() {
            @Override
            protected void beforeHookedMethod(MethodHookParam param) throws Throwable {
                super.beforeHookedMethod(param);
                if (param == null) {
                    return;
                }
                //输出所有参数
                for (Object o : param.args) {
                    Log.d(TAG, className + ",Constructor beforeHookedMethod参数：" + o);
                }
            }

            @Override
            protected void afterHookedMethod(MethodHookParam param) throws Throwable {
                super.afterHookedMethod(param);
                Object obj = param.thisObject;
                Log.d(TAG, className + ",Constructor afterHookedMethod参数：" + obj);
            }
        });

        for (Method method : m) {
            String classMethodName = method.getName();
            String output = "类名：" + className + ",方法名：" + classMethodName + " ";

            XposedBridge.hookMethod(method, new XC_MethodHook() {
                @Override
                protected void beforeHookedMethod(MethodHookParam param) throws Throwable {
                    super.beforeHookedMethod(param);
                    if (param == null) {
                        return;
                    }
                    //输出所有参数
                    for (Object o : param.args) {
                        Log.d(TAG, output + ",beforeHookedMethod参数：" + o);
                    }
                }

                @Override
                protected void afterHookedMethod(MethodHookParam param) throws Throwable {
                    super.afterHookedMethod(param);
                    Object result = param.getResult();
                    Log.d(TAG, output + ",afterHookedMethod返回值：" + result);
                    if (param == null) {
                        return;
                    }
                    //输出所有参数
                    for (int i = 0; i < param.args.length; i++) {
                        Log.d(TAG, output + ",afterHookedMethod参数" + i + "： " + param.args[i]);
                    }
                }
            });
        }
    }

//    private Object getFieldValue(Class obj) {
//        Field field = null;
//        if (obj == null) {
//            return null;
//        }
//        Log.d(TAG,"obj is: " + obj);
//        for(Field f : obj.getDeclaredFields()){
//            f.setAccessible(true);
//            try{
//                field = obj.getDeclaredField(f.getName());
//                Log.d(TAG,"field is: " + field.get(obj));
//            }catch (Exception e){
//                e.printStackTrace();
//            }
//        }
//        return field;
//    }


    private Object getFieldValue(Class<?> obj, String... fieldNames) {
        Object field = null;
        if (obj == null) {
            return null;
        }
        for (String fieldName : fieldNames) {
            field = getFieldValue(obj, fieldName);
        }
        return field;
    }


    private Object getFieldValue(Class<?> obj, String fieldName) {
        try {
            Log.d(TAG, "obj is: " + obj);
            Field field = obj.getDeclaredField(fieldName);
            field.setAccessible(true);
            return field.get(obj);
        } catch (Exception e) {
            e.printStackTrace();
        }
        return null;
    }
    // 第一步：Hook方法ClassLoader#loadClass(String)
//        findAndHookMethod(ClassLoader.class, "loadClass", String.class, new XC_MethodHook() {
//            @Override
//            protected void afterHookedMethod(MethodHookParam param) {
//                if (param.hasThrowable()) return;
//                Class<?> cls = (Class<?>) param.getResult();
//                String name = cls.getName();
//                Log.d(TAG,"loadClass: " + name);
////                if (classes.contains(name)) {
////                    // 所有的类都是通过loadClass方法加载的
////                    // 所以这里通过判断全限定类名，查找到目标类
////                    // 第二步：Hook目标方法
////                    findAndHookMethod(cls, "methodName", paramTypes, new XC_MethodHook() {
////                        @Override
////                        protected void beforeHookedMethod(MethodHookParam param) throws Throwable {
////                            // TODO
////                        }
////
////                        @Override
////                        protected void afterHookedMethod(MethodHookParam param) throws Throwable {
////                            // TODO
////                        }
////                    });
////                }
//            }
//        });
}