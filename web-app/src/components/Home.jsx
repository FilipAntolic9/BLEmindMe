import { useAuth } from '../contexts/AuthContext';
import { useEffect, useCallback } from 'react';
import { getAllTenantDevices, getDeviceLightState } from '../services/DeviceService';
import { requestNotificationPermission } from '../services/NotificationService';
import Navbar from "./NavBar";

const Home = () => {
    const { user } = useAuth();

    const checkDevicesLightState = useCallback(async () => {
        try {
            const response = await getAllTenantDevices(100, 0);
            const devices = response.data;

            for (const device of devices) {
                const lightStates = await getDeviceLightState(device.id.id);

                for (const state of lightStates) {
                    if (state.key.startsWith('lightState_') && state.value === true) {
                        //const username = state.key.split('lightState_')[1];
                        //if (username === user.email.split('@')[0]) {
                            new Notification('BLEmindMe Alert', {
                                body: `Zaboravili ste nešto!`,
                                icon: '/vite.svg'
                            });
                        //}
                    }
                }
            }
        } catch (error) {
            console.error('Error checking device states:', error);
        }
    }, [user]);

    useEffect(() => {
        const setupNotifications = async () => {
            try {
                await requestNotificationPermission();
            } catch (error) {
                console.error('Error setting up notifications:', error);
            }
        };

        setupNotifications();

        const interval = setInterval(checkDevicesLightState, 5000);

        return () => clearInterval(interval);
    }, [checkDevicesLightState]);

    return (
        <div className="min-h-screen bg-gray-100">
            <Navbar />
            <div className="py-12 px-4 sm:px-6 lg:px-8">
                <div className="max-w-3xl mx-auto bg-white rounded-lg shadow-lg p-8">
                    <h1 className="text-3xl font-bold text-gray-900 text-center mb-6">
                        Dobrodošli u BLEmindMe
                    </h1>
                    <p className="text-xl text-gray-700 text-center">
                        {user?.email ? `Prijavljeni ste kao: ${user.email}` : 'Dobrodošli gost'}
                    </p>
                </div>
            </div>
        </div>
    );
};

export default Home;