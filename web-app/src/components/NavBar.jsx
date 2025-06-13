import { useAuth } from '../contexts/AuthContext';
import { useNavigate, Link } from 'react-router-dom';

const Navbar = () => {
    const { user, logout } = useAuth();
    const navigate = useNavigate();

    const handleLogout = () => {
        logout();
        navigate('/login');
    };

    return (
        <nav className="bg-white shadow-lg">
            <div className="max-w-7xl mx-auto px-4">
                <div className="flex justify-between h-16">
                    <div className="flex items-center">
                        <div className="flex-shrink-0">
                            <h1 className="text-xl font-bold">BLEmindMe</h1>
                        </div>
                        {user && (
                            <div className="hidden sm:ml-6 sm:flex sm:space-x-4">
                                <Link
                                    to="/home"
                                    className="text-gray-700 hover:text-blue-600 px-3 py-2 rounded-md text-sm font-medium"
                                >
                                    Početna
                                </Link>
                            </div>
                        )}
                        {user && user.role === 'TENANT_ADMIN' && (
                            <div className="hidden sm:ml-6 sm:flex sm:space-x-4">
                                <Link
                                    to="/device-management"
                                    className="text-gray-700 hover:text-blue-600 px-3 py-2 rounded-md text-sm font-medium"
                                >
                                    Upravljanje uređajima
                                </Link>
                                <Link
                                    to="/device-assignment"
                                    className="text-gray-700 hover:text-blue-600 px-3 py-2 rounded-md text-sm font-medium"
                                >
                                    Dodjela uređaja
                                </Link>
                                <Link
                                    to="/device-mapping"
                                    className="text-gray-700 hover:text-blue-600 px-3 py-2 rounded-md text-sm font-medium"
                                >
                                    Povezivanje uređaja
                                </Link>
                            </div>
                        )}
                        {user && user.role === 'CUSTOMER_USER' && (
                            <div className="hidden sm:ml-6 sm:flex sm:space-x-4">
                                <Link
                                    to="/my-devices"
                                    className="text-gray-700 hover:text-blue-600 px-3 py-2 rounded-md text-sm font-medium"
                                >
                                    Moji uređaji
                                </Link>
                            </div>
                        )}
                    </div>
                    {user && (
                        <div className="flex items-center">
                            <span className="mr-4 text-gray-700">{user.email}</span>
                            <button
                                onClick={handleLogout}
                                className="bg-red-600 hover:bg-red-700 text-white px-4 py-2 rounded-md text-sm font-medium"
                            >
                                Odjava
                            </button>
                        </div>
                    )}
                </div>
            </div>
        </nav>
    );
};

export default Navbar;