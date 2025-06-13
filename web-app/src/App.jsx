import './App.css'
import {AuthProvider} from "./contexts/AuthContext.jsx";
import {NotificationProvider} from "./contexts/NotificationContext.jsx";
import {Route, BrowserRouter as Router, Routes} from "react-router-dom";
import Login from "./components/authentication/Login.jsx";
import Registration from "./components/authentication/Registration.jsx";
import Home from "./components/Home.jsx";
import PrivateRoute from "./components/authentication/PrivateRoute.jsx";
import DeviceManagement from "./components/device/DeviceManagment.jsx";
import TenantAdminRoute from "./components/authentication/TenantAdminRoute.jsx";
import DeviceAssignment from "./components/device/DeviceAssignment.jsx";
import DeviceMapping from "./components/device/DeviceMapping.jsx";
import CustomerUserRoute from "./components/authentication/CustomerUserRoute.jsx";
import CustomerDevices from "./components/device/CustomerDevices.jsx";

function App() {
  return (
      <AuthProvider>
          <NotificationProvider>
              <Router>
                  <div className="App">
                      <Routes>
                          <Route path="/login" element={<Login />} />
                          <Route path="/register" element={<Registration />} />
                          <Route path="/home" element={<PrivateRoute><Home /></PrivateRoute>} />
                          <Route path="/" element={<Login />} />
                          <Route
                              path="/device-management"
                              element={
                                  <PrivateRoute>
                                      <TenantAdminRoute>
                                          <DeviceManagement />
                                      </TenantAdminRoute>
                                  </PrivateRoute>
                              }
                          />
                          <Route
                              path="/device-assignment"
                              element={
                                  <PrivateRoute>
                                      <TenantAdminRoute>
                                          <DeviceAssignment />
                                      </TenantAdminRoute>
                                  </PrivateRoute>
                              }
                          />
                          <Route
                              path="/device-mapping"
                              element={
                                  <PrivateRoute>
                                      <TenantAdminRoute>
                                          <DeviceMapping />
                                      </TenantAdminRoute>
                                  </PrivateRoute>
                              }
                          />
                          <Route
                              path="/my-devices"
                              element={
                                  <PrivateRoute>
                                      <CustomerUserRoute>
                                          <CustomerDevices />
                                      </CustomerUserRoute>
                                  </PrivateRoute>
                              }
                          />
                      </Routes>
                  </div>
              </Router>
          </NotificationProvider>
      </AuthProvider>
  )
}

export default App
