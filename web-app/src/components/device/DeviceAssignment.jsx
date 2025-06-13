import { useState, useEffect } from 'react';
import { useNotification } from '../../contexts/NotificationContext';
import { getAllTenantUsers, assignDeviceToUser } from '../../services/UserService';
import { getAllTenantDevices } from '../../services/DeviceService';
import Navbar from '../NavBar';

const DeviceAssignment = () => {
    const [users, setUsers] = useState([]);
    const [devices, setDevices] = useState([]);
    const [selectedUser, setSelectedUser] = useState('');
    const [selectedDevice, setSelectedDevice] = useState('');
    const [userPage, setUserPage] = useState(0);
    const [devicePage, setDevicePage] = useState(0);
    const [hasMoreUsers, setHasMoreUsers] = useState(true);
    const [hasMoreDevices, setHasMoreDevices] = useState(true);
    const pageSize = 10;

    const { showNotification } = useNotification();

    const loadUsers = async (page) => {
        try {
            const response = await getAllTenantUsers(pageSize, page);
            if (page === 0) {
                setUsers(response.data);
            } else {
                setUsers(prev => [...prev, ...response.data]);
            }
            setHasMoreUsers(response.data.length === pageSize);
        } catch (err) {
            showNotification('Greška prilikom dohvaćanja korisnika: ' + err.message, 'error');
        }
    };

    const loadDevices = async (page) => {
        try {
            const response = await getAllTenantDevices(pageSize, page);
            if (page === 0) {
                setDevices(response.data);
            } else {
                setDevices(prev => [...prev, ...response.data]);
            }
            setHasMoreDevices(response.data.length === pageSize);
        } catch (err) {
            showNotification('Greška prilikom dohvaćanja uređaja: ' + err.message, 'error');
        }
    };

    useEffect(() => {
        loadUsers(0);
        loadDevices(0);
    }, []);

    const handleAssignment = async (e) => {
        e.preventDefault();
        if (!selectedUser || !selectedDevice) {
            showNotification('Molimo odaberite korisnika i uređaj', 'error');
            return;
        }

        try {
            await assignDeviceToUser(selectedUser, selectedDevice);
            showNotification('Uređaj uspješno dodijeljen korisniku!', 'success');
            setSelectedUser('');
            setSelectedDevice('');
        } catch (err) {
            showNotification('Greška prilikom dodjeljivanja uređaja: ' + err.message, 'error');
        }
    };

    return (
        <div className="min-h-screen bg-gray-100">
            <Navbar />
            <div className="max-w-2xl mx-auto mt-10 bg-white p-8 rounded-lg shadow">
                <h2 className="text-2xl font-bold mb-6 text-center">Dodijeli uređaj korisniku</h2>
                <form onSubmit={handleAssignment} className="space-y-6">
                    <div>
                        <label className="block text-sm font-medium text-gray-700 mb-2">Odaberi korisnika</label>
                        <select
                            value={selectedUser}
                            onChange={(e) => setSelectedUser(e.target.value)}
                            className="w-full p-2 border rounded-md"
                            required
                        >
                            <option value="">Odaberi korisnika...</option>
                            {users.map(user => (
                                <option key={user.id.id} value={user.customerId.id}>
                                    {user.email} ({user.firstName} {user.lastName})
                                </option>
                            ))}
                        </select>
                        {hasMoreUsers && (
                            <button
                                type="button"
                                onClick={() => {
                                    setUserPage(prev => prev + 1);
                                    loadUsers(userPage + 1);
                                }}
                                className="mt-2 text-blue-600 hover:text-blue-800"
                            >
                                Učitaj više korisnika...
                            </button>
                        )}
                    </div>

                    <div>
                        <label className="block text-sm font-medium text-gray-700 mb-2">Odaberi uređaj</label>
                        <select
                            value={selectedDevice}
                            onChange={(e) => setSelectedDevice(e.target.value)}
                            className="w-full p-2 border rounded-md"
                            required
                        >
                            <option value="">Odaberi uređaj...</option>
                            {devices.map(device => (
                                <option key={device.id.id} value={device.id.id}>
                                    {device.name} ({device.type})
                                </option>
                            ))}
                        </select>
                        {hasMoreDevices && (
                            <button
                                type="button"
                                onClick={() => {
                                    setDevicePage(prev => prev + 1);
                                    loadDevices(devicePage + 1);
                                }}
                                className="mt-2 text-blue-600 hover:text-blue-800"
                            >
                                Učitaj više uređaja...
                            </button>
                        )}
                    </div>

                    <button
                        type="submit"
                        className="w-full bg-blue-600 text-white py-2 px-4 rounded-md hover:bg-blue-700 focus:outline-none focus:ring-2 focus:ring-blue-500 focus:ring-offset-2"
                    >
                        Dodijeli uređaj
                    </button>
                </form>
            </div>
        </div>
    );
};

export default DeviceAssignment;