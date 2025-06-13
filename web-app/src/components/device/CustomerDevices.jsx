import { useState, useEffect } from 'react';
import { useNotification } from '../../contexts/NotificationContext';
import { useAuth } from '../../contexts/AuthContext';
import { getAllCustomerDevices, getDevicesRelations } from '../../services/DeviceService';
import Navbar from '../NavBar';

const CustomerDevices = () => {
    const [devices, setDevices] = useState([]);
    const [selectedDevice, setSelectedDevice] = useState(null);
    const [relations, setRelations] = useState([]);
    const [page, setPage] = useState(0);
    const [hasMore, setHasMore] = useState(true);
    const [loading, setLoading] = useState(false);
    const pageSize = 10;

    const { user } = useAuth();
    const { showNotification } = useNotification();

    const loadDevices = async (currentPage) => {
        try {
            setLoading(true);
            const response = await getAllCustomerDevices(user.customerId, pageSize, currentPage);

            if (currentPage === 0) {
                setDevices(response.data);
            } else {
                setDevices(prev => [...prev, ...response.data]);
            }

            setHasMore(response.data.length === pageSize);
        } catch (err) {
            showNotification('Greška prilikom dohvaćanja uređaja: ' + err.message, 'error');
        } finally {
            setLoading(false);
        }
    };

    const loadRelations = async (deviceId, deviceType) => {
        try {
            const direction = deviceType === 'ESP32' ? 'FROM' : 'TO';
            const relations = await getDevicesRelations(deviceId, direction);
            setRelations(relations);
        } catch (err) {
            showNotification('Greška prilikom dohvaćanja relacija: ' + err.message, 'error');
        }
    };

    useEffect(() => {
        loadDevices(0);
    }, []);

    const handleDeviceClick = async (device) => {
        setSelectedDevice(device);
        const deviceType = device.deviceProfileId.id === "76042e60-4150-11f0-a544-db21b46190ed" ? 'TAGS' : 'ESP32';
        await loadRelations(device.id.id, deviceType);
    };

    return (
        <div className="min-h-screen bg-gray-100">
            <Navbar />
            <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 py-8">
                <h2 className="text-2xl font-bold mb-6 text-center">Moji uređaji</h2>

                <div className="grid grid-cols-1 md:grid-cols-2 gap-6">
                    {/* Devices List */}
                    <div className="bg-white p-6 rounded-lg shadow">
                        <h3 className="text-xl font-semibold mb-4">Lista uređaja</h3>
                        <div className="space-y-4">
                            {devices.map(device => (
                                <div
                                    key={device.id.id}
                                    onClick={() => handleDeviceClick(device)}
                                    className={`p-4 border rounded-lg cursor-pointer hover:bg-gray-50 ${
                                        selectedDevice?.id.id === device.id.id ? 'border-blue-500 bg-blue-50' : 'border-gray-200'
                                    }`}
                                >
                                    <h4 className="font-medium">{device.name}</h4>
                                    <p className="text-sm text-gray-600">{device.type}</p>
                                    <p className="text-sm text-gray-500">MAC: {device.label}</p>
                                </div>
                            ))}

                            {hasMore && (
                                <button
                                    onClick={() => {
                                        setPage(prev => prev + 1);
                                        loadDevices(page + 1);
                                    }}
                                    disabled={loading}
                                    className="w-full py-2 text-center text-blue-600 hover:text-blue-800 disabled:text-gray-400"
                                >
                                    {loading ? 'Učitavam...' : 'Učitaj više'}
                                </button>
                            )}
                        </div>
                    </div>

                    {/* Device Details and Relations */}
                    <div className="bg-white p-6 rounded-lg shadow">
                        <h3 className="text-xl font-semibold mb-4">Detalji uređaja</h3>
                        {selectedDevice ? (
                            <div>
                                <div className="mb-6">
                                    <h4 className="font-medium mb-2">Informacije o uređaju</h4>
                                    <p><span className="font-medium">Naziv:</span> {selectedDevice.name}</p>
                                    <p><span className="font-medium">Tip:</span> {selectedDevice.type}</p>
                                    <p><span className="font-medium">MAC adresa:</span> {selectedDevice.label}</p>
                                </div>

                                <div>
                                    <h4 className="font-medium mb-2">
                                        {selectedDevice.deviceProfileId.id === "76042e60-4150-11f0-a544-db21b46190ed"
                                            ? 'Povezan sa ESP32 uređajem:'
                                            : 'Povezani tagovi:'}
                                    </h4>
                                    {relations.length > 0 ? (
                                        <ul className="space-y-2">
                                            {relations.map(relation => (
                                                <li key={relation.id.id} className="p-2 bg-gray-50 rounded">
                                                    {relation.name} - {relation.type}
                                                </li>
                                            ))}
                                        </ul>
                                    ) : (
                                        <p className="text-gray-500">Nema povezanih uređaja</p>
                                    )}
                                </div>
                            </div>
                        ) : (
                            <p className="text-gray-500">Odaberite uređaj za prikaz detalja</p>
                        )}
                    </div>
                </div>
            </div>
        </div>
    );
};

export default CustomerDevices;